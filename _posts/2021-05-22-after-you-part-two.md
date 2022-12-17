---
layout: post
title: 'Overly interoperable libraries, part 2'
date: 2021-05-22 00:01:00 +0000
tags:
  implementation-divergence
  metaprogramming
  cpplang-slack
---

I've received a few proposed solutions re
["Overly interoperable libraries"](/blog/2021/05/19/after-you-no-after-you/) (2021-05-19).
It's become clear that I set the initial goalposts a little too close
to the kicker. :)

My post asked for a solution to "the general problem," but provided only a relatively
trivial example with two client libraries:

- `Foolib` provides `class Foo`, and wants `operator^` to exist whenever the left-hand
    operand is `Foo` and the right-hand operand `U` defines `U::foolib_compatible`.

- `Barlib` provides `class Bar`, and wants `operator^` to exist whenever the right-hand
    operand is `Bar` and the left-hand operand `T` defines `T::barlib_compatible`.


## Easy goal number 1

Peter Dimov and Piotr Nycz presented solutions based on the fact that my sample `Foolib`
cared mainly about the left-hand operand, and my sample `Barlib` cared mainly about the
right-hand operand ([Godbolt](https://godbolt.org/z/P4dM5aoe4)):

    template<class T, class U>
    struct enable_xor : std::disjunction<enable_xor_lhs<T, U>,
                                         enable_xor_rhs<T, U>> {};

    ~~~

    template<class U>  // Foo can be xor'ed with anything foolib-compatible.
    struct enable_xor_lhs<Foolib::Foo, U, std::enable_if_t<U::foolib_compatible>>
        : std::true_type {};

    ~~~

    template<class T>  // Bar can be xor'ed with anything barlib-compatible.
    struct enable_xor_rhs<T, Barlib::Bar, std::enable_if_t<T::barlib_compatible>>
        : std::true_type {};

This approach immediately breaks down if both `Foolib` and `Barlib` care about the
left-hand operand; for example if we rewrite the requirements to ([Godbolt](https://godbolt.org/z/Tz6sWj7c1)):

- `Foolib` provides `class Foo`, and wants `operator^` to exist whenever the left-hand
    operand is <b>derived from</b> `Foo` and the right-hand operand `U` defines `U::foolib_compatible`.

- `Barlib` provides `class Bar`, and wants `operator^` to exist whenever the <b>left-hand</b>
    operand is <b>derived from</b> `Bar` and the right-hand operand `U` defines `U::barlib_compatible`.

Thus:

    struct X : Foolib::Foo, Barlib::Bar { int value() const; } x;
    int main() {
        return (x ^ x);  // Collision again!
    }


## Easy goal number 2: The orphan rule

Chase Albert and Jonathan Müller pointed out that this problem is related to what Rust calls the
["orphan rule."](https://smallcultfollowing.com/babysteps/blog/2015/01/14/little-orphan-impls/)
Our "`operator^`-enablement" is what Rust would call a "trait" — a common behavior that types
can opt-into.
I don't know much Rust, but as I understand it, the essence of the orphan rule is that only two
people have the authority to "opt a type in" to a trait: the type author, and the trait author.
Other components should butt out.

This is a great principle for practical code — so great that my given example adhered to it without
consciously intending to. `Foolib`'s author opted-into `operator^` only when at least one of the operands
was in fact a type associated with `namespace Foolib`. `Barlib`'s author opted-into `operator^` only
when at least one of the operands was in fact a type associated with `namespace Barlib`.

C++'s argument-dependent lookup ([ADL](/blog/2019/04/26/what-is-adl/)) takes the same approach:
an unqualified `fight(you, him)` will look up functions named `fight` in namespaces
_associated with_ the types of `you` and/or `him`, but a completely unrelated `namespace wimpy`
cannot [butt in with its own suggestion](https://tvtropes.org/pmwiki/pmwiki.php/Main/LetsYouAndHimFight).

Jonathan Müller worked up a more or less complete solution based on this approach — basically, recapitulating
the ADL associated-namespace rules in partial-specialization form. I've massaged it a bit for
presentation here. ([Godbolt](https://godbolt.org/z/da4bMhfb7).)

    template<class T> constexpr auto libs_of = tl<T>{};
    template<class T> constexpr auto libs_of<const T> = libs_of<T>;
    template<class T> constexpr auto libs_of<volatile T> = libs_of<T>;
    template<class T> constexpr auto libs_of<T*> = libs_of<T>;
    template<class T> constexpr auto libs_of<T&> = libs_of<T>;
    template<class T> constexpr auto libs_of<T&&> = libs_of<T>;
    template<template<class...> class C, class... As> constexpr auto libs_of<C<As...>> = (tl<>{} + ... + libs_of<As>);
    template<class R, class... As> constexpr auto libs_of<R(As...)> = (libs_of<R> + ... + libs_of<As>);

    template<class T, class U, class LibList>
    struct enable_if_xor_is_enabled;
    template<class T, class U, class... Libs>
    struct enable_if_xor_is_enabled<T, U, tl<Libs...>>
        : std::enable_if<(enable_xor<Libs, T, U>::value || ...), int> {};

    template<class T, class U,
        typename enable_if_xor_is_enabled<T, U, decltype(libs_of<T> + libs_of<U>)>::type = 0>
    int operator^(T t, U u) {
        return t.value() ^ u.value();
    }

    ~~~

    template<class U>  // Foo can be xor'ed with anything foolib-compatible.
    struct enable_xor<Foolib::Foo, Foolib::Foo, U, std::enable_if_t<U::foolib_compatible>>
        : std::true_type {};

Unfortunately this approach cannot handle the "derived from `Foo`" example above,
because the generic `libs_of` has no way to enumerate the public base classes of
`X` and discover that it should be looking for `enable_xor` specializations associated
with `Foo` and `Bar`. In other words, our `libs_of` metaprogramming cannot ever _fully_
recapitulate the rules of ADL.

Alternatively, we could require each type `X` to specialize `libs_of<X>` and explicitly
name the libraries with which it wants to be associated. But I claim that that idea
(1) doesn't scale, and (2) doesn't satisfy the problem statement. (We said `Foolib`
wants `operator^` to exist whenever the left-hand operand is derived from `Foo`. We
didn't say "whenever the left-hand operand is derived from `Foo` and also specializes
`libs_of`.")


## Analogy to smart pointers

The intent of my metaprogramming puzzle is essentially to find a solution that implements
"shared ownership" of `operator^`, in a sense that is exactly analogous
to `shared_ptr`'s "shared ownership" of a heap allocation. We have arbitrarily many
entities here, each of which has an equal stake in the existence of `operator^`; just as
with `shared_ptr` we have arbitrarily many entities, each with an equal stake
in the existence of some heap-allocated object.

Rust's "orphan rule" is analogous to replacing `shared_ptr` with `unique_ptr`. The orphan rule
highlights that truly "shared" ownership is rarely necessary. We can usually identify
a single privileged "owner" for our heap-allocation, and then we can use `unique_ptr` instead
of `shared_ptr`, and then our code will be easier to reason about.

This is absolutely true! _Especially_ in production code! And usually it turns out to be
pretty easy to refactor shared ownership into unique ownership, as Jonathan did in his solution.

However, _for the purposes of this puzzle_, that's "cheating." Even if the `libs_of` approach
were able to fully recapitulate all of C++'s "associated namespace" rules, it still wouldn't
be able to deal with a `namespace wimpy` that wants to enable `operator^` for types not
concretely associated with it, e.g.

- `Wimpylib` wants `operator^` to exist whenever _either_ the left-hand operand `T` defines
    `T::wimpy_compatible` _or_ the right-hand operand `U` defines `U::wimpy_compatible`
    (or both).

My GUID-based toy solution handles this case fine ([Godbolt](https://godbolt.org/z/3bzcobxK8)):

    template<class T, class U, std::enable_if_t<T::wimpy_compatible, int> = 0>
    int enable_xor(priority_tag<22>);  // Wimpy needs two GUIDs; one is "22"

    template<class T, class U, std::enable_if_t<U::wimpy_compatible, int> = 0>
    int enable_xor(priority_tag<23>);  // and the other is "23"


## Perf improvements to the GUID solution

Jonathan Müller offered an alternative GUID solution: Use
C++17 fold-expressions to "iterate instead of recurse."
Instead of ([Godbolt](https://godbolt.org/z/sMqv6a5rs))

    template<class, class>
    void enable_xor(priority_tag<0>);

    template<class T, class U,
             decltype(enable_xor<T, U>(priority_tag<99>{})) = 0>
    int operator^(T t, U u) {
        return t.value() ^ u.value();
    }

we can use a fold-expression ([Godbolt](https://godbolt.org/z/TcG7h7ffc)):

    template<size_t K> struct index_tag {};
    template<class, class> void enable_xor(index_tag<0>) = delete;

    template<size_t I, class T, class U, class = void>
    constexpr bool has_enabler = false;
    template<size_t I, class T, class U>
    constexpr bool has_enabler<I, T, U, decltype(enable_xor<T,U>(index_tag<I>()), void())> = true;

    template<class T, class U, size_t... Is>
    constexpr bool should_enable_xor(std::index_sequence<Is...>) {
        return (has_enabler<Is, T, U> || ...);
    };

    template<class T, class U,
             std::enable_if_t<should_enable_xor<T, U>(std::make_index_sequence<99>()), int> = 0>
    int operator^(T t, U u) {
        return t.value() ^ u.value();
    }

> Confusingly, both libc++ and libstdc++ fail to handle this exact solution
> when `index_tag` is rewritten as ([Godbolt](https://godbolt.org/z/1TnYPWG9M))
>
>     template<size_t K>
>     using index_tag = std::integral_constant<size_t, K>;
>
> I don't know what's going wrong there. MSVC (and/or MSVC STL) handles it just fine.

Switching to fold-expressions is unfortunately not a _strict_ win, because of Clang. Clang's
implementation-defined limit on the size of a fold-expression is far less than its implementation-defined
limit on recursive template instantiations. GCC and ICC, meanwhile, seem not to
limit the size of fold-expressions.

But this reminded me of another classic technique for implementing `conjunction` and `disjunction`:
Use `is_same` to do the heavy lifting. To test whether `Bs...` are all `true`, we simply
compare the type `int(bool_constant<Bs>...)` against the type `int(bool_constant<true>...)`.
Now we have no recursive templates _and_ no large expressions; we simply have a type with a large parameter list.
([Godbolt.](https://godbolt.org/z/aaEj577c6))

    template<bool... Bs>
    using And = std::is_same<int(index_tag<Bs>...),
                             int(index_tag<Bs||true>...)>;

    template<class T, class U, size_t... Is>
    constexpr bool should_enable_xor(std::index_sequence<Is...>) {
        return !And<!has_enabler<Is, T, U> ...>::value;
    };

This is far and away the winning strategy on today's compilers; no compiler bothers to set a detectable limit
on "number of function parameters in a function type."

|----------------|-----|-------|------|-----|
| Technique      | GCC | Clang | MSVC | ICC |
|----------------|-----|-------|------|-----|
| [`priority_tag`](https://godbolt.org/z/sMqv6a5rs) | 900 | 1025 | 497 | 500 |
| [`index_tag` + fold](https://godbolt.org/z/TcG7h7ffc) | 5000+ | 256 | 597 | 33000+ |
| [`index_tag` + same](https://godbolt.org/z/aaEj577c6) | 20000+ | 50000+ | 35000+ | 30000+ |

So now we've got a GUID-based solution permitting four-digit GUIDs.
Can we do any better?

----

See also:

* ["Overly interoperable libraries, part 3"](/blog/2021/05/26/after-you-solution/) (2021-05-26)
