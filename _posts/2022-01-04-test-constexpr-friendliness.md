---
layout: post
title: "Test an expression for constexpr-friendliness"
date: 2022-01-04 00:01:00 +0000
tags:
  constexpr
  implementation-divergence
  metaprogramming
  cpplang-slack
---

Via Mark de Wever on the
[cpplang Slack](https://cppalliance.org/slack/): Mark demonstrates
how to tell whether a snippet of code is constexpr-friendly or not.
([Godbolt.](https://godbolt.org/z/xfcGGed1n))

    template<class F, int = (F{}(), 0)>
    constexpr bool is_constexpr_friendly(F) { return true; }
    constexpr bool is_constexpr_friendly(...) { return false; }

    int g = 42;
    constexpr int test(int x) {
        return (x == 42) ? g : x;
    }
    static_assert(!is_constexpr_friendly([]{ test(42); }));
    static_assert(is_constexpr_friendly([]{ test(43); }));

Notice that this technique relies on default-constructing
(at compile time) an instance of the stateless lambda type `F`;
the ability to default-construct stateless lambda types is new
in C++20.

In the code above, `test(42)` is not a constant expression.
So `(F{}(), 0)` is not a constant expression. So it can't be
used as a template argument. So substitution of `F` into
`is_constexpr_friendly` #1 fails; substitution failure is not
an error; `is_constexpr_friendly` #2 is chosen by overload
resolution.

C++20 also introduces `consteval` functions — which are like
`constexpr` functions but _must_ produce compile-time results —
and Mark found that the above technique failed when `test`
was `consteval`.

    template<class F, int = (F{}(), 0)>
    constexpr bool is_constexpr_friendly(F) { return true; }
    constexpr bool is_constexpr_friendly(...) { return false; }

    int g = 42;
    consteval int test2(int x) {
        return (x == 42) ? g : x;
    }
    static_assert(!is_constexpr_friendly([]{ test2(42); }));
    static_assert(is_constexpr_friendly([]{ test2(43); }));

All three major vendors error out here ([Godbolt](https://godbolt.org/z/oq5znKn19)).
MSVC's error message is representative:

    error C7595: 'test2': call to immediate function is not a constant expression
    note: failure was caused by non-constant arguments or reference to a non-constant symbol
    note: see usage of 'g'

In the code above, `test2(42)` is an invocation of a `consteval`
function, but it's not a constant expression. That's a hard error.
The compiler's chain of logic stops there; we never get as far as
overload resolution.

Notice that `test2` remains a valid `consteval` function definition!
It's simply ill-formed to pass it `42`.

----

Johel Ernesto Guerrero Peña provides an alternative idiom.
He puts the questionably-constant expression in an explicitly provided
template argument instead of a defaulted template argument,
and uses `requires` to create the SFINAE context.
([Godbolt](https://godbolt.org/z/v5oEKrY1z).)

    template<int> struct A {};

    template<class F>
    constexpr bool is_constexpr_friendly(F) {
        return requires {
            typename A<(F{}(), 1)>;
        };
    }

    static_assert(!is_constexpr_friendly([]{ test(42); }));
    static_assert(is_constexpr_friendly([]{ test(43); }));

However, again, this fails to work for `consteval` functions
like `test2`.

----

I'm not aware of any technique to "safely simulate" the evaluation
of a `consteval` function and bail out in a SFINAE-friendly manner
if it hits a problem. My impression is that this is intentional:
`consteval` functions are _designed_ as a kind of opaque box where
errors are errors and the compiler needn't do anything "tentatively."
If you wanted `constexpr`'s fall-back-to-runtime behavior, you would
just use `constexpr`; if you're using `consteval`, it's because
you _want_ hard errors when your `consteval` function is misused.

However, if you think you know a trick to tentatively evaluate
a `consteval` function, I'll be interested to hear about it!


## Sidenote 1: `consteval` functions are like `constexpr` variables

I noticed that the difference between `constexpr` and `consteval`
functions is basically the same as the difference between `const`
and `constexpr` variable templates ([Godbolt](https://godbolt.org/z/xzsG3z6fz)):

    int g = 42;

    constexpr int test(int x)
        { return x == 42 ? g : x; }

    consteval int test2(int x)
        { return x == 42 ? g : x; }

is analogous to

    template<int X>
    const int vtest = (X == 42) ? g : X;

    template<int X>
    constexpr int vtest2 = (X == 42) ? g : X;

The expressions `test(43)` and `vtest<43>` are compile-time constant
expressions. The expressions `test(42)` and `vtest<42>` are legal C++,
both equal to `42` at runtime, but _not_ compile-time constants.

The expressions `test2(43)` and `vtest2<43>`, likewise, are compile-time
constants. But `test2(42)` is a hard error: a `consteval` function must not
return a runtime result. And `vtest2<42>` is a hard error: a `constexpr`
variable must not be initialized with a runtime value.


## Sidenote 2: Implementation divergence on template default arguments

Naturally, I tried to mechanically transform Mark's code using `constexpr`
and `consteval` functions, into the corresponding code using `const` and
`constexpr` variables. But I messed it up — twice! First I wrote this:

    int g = 42;

    #ifndef CONSTEXPR
        template<int X>
        const int vtest = (X == 42) ? g : X;
    #else
        template<int X>
        constexpr int vtest2 = (X == 42) ? g : X;
    #endif

    template<class F, int = vtest<42>>
    constexpr bool is_constexpr_friendly(F&&) { return true; }
    constexpr bool is_constexpr_friendly(...) { return false; }

    static_assert(!is_constexpr_friendly());

I was surprised to see three different behaviors from the three
major compiler vendors:
MSVC accepts this program with or without `-DCONSTEXPR`.
GCC rejects it only with `-DCONSTEXPR`. Clang rejects it unconditionally.
([Godbolt.](https://godbolt.org/z/qxPYT613v))

Clang's error message tells me my first stupid mistake:

    error: use of undeclared identifier 'vtest'
    template<class F, int = vtest<42>>
                            ^

Oh, right. I defined `vtest` in the first branch of the `#ifdef` but
`vtest2` in the second branch. That was silly. Let's fix that.

    int g = 42;

    #ifndef CONSTEXPR
        template<int X>
        const int vtest = (X == 42) ? g : X;
    #else
        template<int X>
        constexpr int vtest = (X == 42) ? g : X;  // !!
    #endif

    template<class F, int = vtest<42>>
    constexpr bool is_constexpr_friendly(F&&) { return true; }
    constexpr bool is_constexpr_friendly(...) { return false; }

    static_assert(!is_constexpr_friendly());

The three major vendors _still_ have three different behaviors,
but they've switched places! Now, MSVC rejects the program only
with `-DCONSTEXPR`; GCC unconditionally accepts; and Clang continues
to reject unconditionally.
([Godbolt.](https://godbolt.org/z/714zMvTrz))

Anyway, this code is still messed up, and not doing what I intended
at all... because there's no way overload resolution will ever pick
`is_constexpr_friendly` #1! It takes an argument of type `F&&`, but
the caller isn't passing any arguments at all. Silly mistake!
The implementation divergence above can be reduced to this
([Godbolt](https://godbolt.org/z/6ocx8791n)):

    template<int = vtest<42>>
    struct A {};

It appears that GCC waits until `A<int>` is actually used before
evaluating the default argument at all; Clang eagerly requires
the default argument `vtest<42>` to be a constant expression even
if it's never used; and MSVC eagerly instantiates `vtest<42>` but
doesn't require it to have a constant value if it's never used.
MSVC's behavior seems friendliest in this case, but from a compiler
dev's point of view it's arguably the least explicable: Why bother
to eagerly instantiate `vtest<42>` at all, if you're not planning
to check that it is a constant expression of the appropriate type?
