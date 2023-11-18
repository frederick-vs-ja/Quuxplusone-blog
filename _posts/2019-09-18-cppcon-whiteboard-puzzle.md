---
layout: post
title: "Seen on the CppCon whiteboard"
date: 2019-09-18 00:01:00 +0000
tags:
  concepts
  conferences
  cppcon
  cpplang-slack
---

Seen on the whiteboard outside the Aurora D conference room at CppCon 2019:

> Can I write a concept that matches any type that has a member function
> with a given signature?

And then underneath:

    template<class T>
    concept Foo = requires {
        int T::foo(double);  // not real syntax
    };

And then underneath _that:_

> EDIT: Got my answer, thanks!

Which by the way is basically the most useless edit you can make to any technical query.
If you're going to claim to have gotten your answer, at least repeat that answer for posterity!
This applies just as much to whiteboards as to StackOverflow.

Anyway, I wrote "can't be done," which generated some fun discussion just now.
I'll present the discussion separated by horizontal lines, with spoilers below
each line.

> I guess I should mention that another correct response is "You shouldn't try to do that,"
> or "you're asking the wrong question." C++2a Concepts aren't designed to discriminate
> based on function signatures; they're designed to deal with syntactic well-formedness
> of expressions. But it's still fun, and perhaps educational, to try to answer the
> question as it was originally asked.

----

Step one: let's try to use valid C++2a syntax!

    template<class T>
    concept Foo = requires (T t) {
        t.foo(1.0) -> int;
    };

GCC and Clang both complain:

        t.foo(1.0) -> int;
                      ^
    error: expected unqualified-id

Do you see the problem? Spoiler below the line.

----

The problem with that first attempt is that C++2a syntax requires curly braces around
any expression whose result type is being constrained. What we've got there is a simple
requirement that the expression `t.foo(1.0)->int` must be well-formed. Good thing we
didn't want to match `size_t` instead of `int`! It would have compiled quietly and
matched essentially no types. (This was one of my chest-mimic examples
in ["Concepts As She Is Spoke"](https://www.youtube.com/watch?v=CXn02MPkn8Y) (CppCon 2018).)

Okay, step two: use _actually_ valid C++2a syntax.

    template<class T>
    concept Foo = requires (T t) {
        { t.foo(1.0) } -> int;
    };

    struct S {
        int foo(double);
    };
    static_assert(Foo<S>);

This syntax compiles, and does _essentially_ the right thing. This is the sort of thing
that I would expect ordinary users of concepts to write, and this is the sort of thing
that C++2a's designers expect you to write.

Yet I said that OP's question "can't be done." Because testing _syntactic validity_ is not
the same thing as matching, quote, "any type that has a _member function_
with a given _signature._" Spoiler below the line.

----

    struct S {
        static char (*foo)(int, ...);
    };
    static_assert(Foo<S>);

This `S` has no member functions. It has a _static data member_ of function-pointer type.
The pointed-to function has a signature whose parameters are not `(double)` but `(int, ...)`.
Its return type is `char`, not `int`. But it still matches our concept!

We can fix the last, smallest, problem by writing `-> std::same<int>` instead of simply `-> int`.
The `-> some-concrete-type` syntax is essentially a trap; no Standard Library components are
specified using that syntax because it is so rarely what you want. I wouldn't be surprised
if it is dropped from C++2a. (*C++20 UPDATE: Reader, [it was](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1084r2.pdf).*)

Anyway, our step-two concept also accepts some types that would be rejected by a strict reading
of OP's question, even though in the real world we'd probably be happy to accept them for
template-metaprogramming purposes. For example:

    struct S {
        template<class A>
        int foo(A) const noexcept;
    };

This `S` has no member functions; it has a non-static member function _template_. Furthermore,
the signature of the function instantiated from that template is const-qualified —
`int S::foo(double) const`, not simply `int S::foo(double)`. Furthermore, in C++17 even the
`noexcept`-specifier is part of the function's signature. (Or at least it's part of its type.
I don't know if "signature" and "type" should be considered synonymous in this context.)

Next, Vaughn Cato and Anthony Williams pointed out a sneakier way to solve OP's problem:

    template<class T, int (T::*)(double)>
    struct helper {};

    template<class T>
    concept Foo = requires (T t) {
        { helper<T, &T::foo>{} };
    };

    struct S {
        int foo(double);
    };
    static_assert(Foo<S>);

    struct S2 {
        int foo(double) const;
    };
    static_assert(not Foo<S2>);  // hooray!

Notice that for backward compatibility, C++17 had to make `noexcept` function pointers
implicitly convertible to non-`noexcept` function pointers. So we also have this:

    struct S3 {
        int foo(double) noexcept;
    };
    static_assert(Foo<S3>);

However, we are still testing _syntactic validity_, not testing for a "member function with
a given signature." Do you see how to break this version? Spoiler below the line.

----

Library maintainers may have smelled out the sneaky solution, because they always have to be
on the lookout for explicit use of `operator&`. The sneaky `S` in this case is

    struct EvilS;
    struct Evil {
        constexpr auto operator&() const -> int (EvilS::*)(double) { return 0; }
    };
    struct EvilS {
        static constexpr Evil foo {};
    };
    static_assert(Foo<EvilS>);

And unfortunately we cannot replace `&T::foo` with `std::addressof(T::foo)` because
C++ does not allow `T::foo` to exist separately from an object when `T::foo` _is_ a
member function.

But this suggests a further [epicyclical](https://en.wikipedia.org/wiki/Deferent_and_epicycle)
fix we can make to our concept! Do you see it?

----

The fix I'm thinking of is:

    template<class T>
    concept NotFoo = requires (T t) {
        { helper<T, &(T::foo)>{} };
    };

    template<class T>
    concept Foo = requires (T t) {
        { helper<T, &T::foo>{} };
    } && !NotFoo<T>;

Now `concept Foo` correctly rejects our `EvilS`, _because_ `&(EvilS::foo)` is permitted,
whereas well-behaved models of our concept permit `&T::foo` but _not_ `&(T::foo)`.

We still have to find a way to reject member function templates which can be
_instantiated_ through type deduction to match the signature `int (S::*)(double)`.

    struct S {
        template<class T> int foo(T);
    };
    static_assert(Foo<S>);  // oops!

Spoiler below the line.

----

As far as I know, any function template that's callable as `t.foo(x)` is also
callable as `t.foo<>(x)`. The angle brackets just mean "here come the template arguments,
if any"; it's totally fine to provide none. So we can test for whether `foo` is a template
similarly to how we tested if it was a static data member:

    template<class T>
    concept AlsoNotFoo = requires (T t) {
        { helper<T, &T::template foo<>>{} };
    };

    template<class T>
    concept Foo = requires (T t) {
        { helper<T, &T::foo>{} };
    } && !NotFoo<T> && !AlsoNotFoo<T>;

Unfortunately, _this_ concept is too _restrictive_ — do you see how?

----

We might have _both_ a `foo` member function template and a `foo` member function with
the appropriate signature!

    struct S {
        template<class T> int foo(T);
        int foo(double);
    };
    static_assert(not Foo<S>);  // oops!

Since `&S::template foo<>` is well-formed, our latest concept rejects this `S`; but in
fact this `S` _does_ have a member function with the signature `int foo(double)`, so we
shouldn't have rejected it at all!

Can you figure out the next step?

[Here's a Godbolt](https://concepts.godbolt.org/z/NtXgkn) of the test cases so far.

----

UPDATE: Circa April 2020, Jason Cobb came up with this solution that correctly
accepts our `S` above.

    template<class T, int (T::*first)(double), int (T::*second)(double)>
    concept NotEq = (first != second);

    template<class T>
    concept Foo = requires (T t) {
        { helper<T, &T::foo>{} };
    } && !NotFoo<T> && (!AlsoNotFoo<T> || NotEq<T, &T::foo, &T::template foo<>>);

This clever piece of code checks (at compile time) to see whether converting `&T::foo`
to a member function pointer produces the same pointer as converting `&T::foo<>`.
There are three possibilities here:

* If `&T::foo<>` is ill-formed, then we have no template member at all, and we want to accept.
      (This is the `!AlsoNotFoo<T>` case.)

* If `&T::foo<> == &T::foo`, then there is a template member,
      and `&T::foo` _also_ refers to the template member, so there must be no _non_-template member,
      and so we want to reject.

* If `&T::foo<> != &T::foo`, then there is a template member, but there is also a
      different non-template member, so we should accept.

Looking for further loopholes, I see that the original question asked to match
any type that _has_ a member function with the given signature; it's not clear
whether this is intended to include _inherited_ member functions or not.
The solution we've been building since the introduction of `helper<T, &T::foo>`
has a problem: it doesn't work for inherited member functions.

    struct Base { int foo(double); };
    struct Inherited : Base {};
    static_assert(not Foo<Inherited>);  // oops!

Can you figure out the next step, if there is one?

[Here's a Godbolt](https://concepts.godbolt.org/z/-_rrop) of the test cases so far.
