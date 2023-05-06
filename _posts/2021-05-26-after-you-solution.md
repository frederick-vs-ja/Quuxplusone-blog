---
layout: post
title: 'Overly interoperable libraries, part 3'
date: 2021-05-26 00:01:00 +0000
tags:
  argument-dependent-lookup
  cpplang-slack
  library-design
  metaprogramming
excerpt: |
  Previously on this blog:

  * ["A metaprogramming puzzle: Overly interoperable libraries"](/blog/2021/05/19/after-you-no-after-you/) (2021-05-19)

  * ["Overly interoperable libraries, part 2"](/blog/2021/05/22/after-you-part-two/) (2021-05-22)

  Jason Cobb has sent me a proposal that I think meets all the requirements and
  therefore qualifies as a complete solution!

  Unlike the ideas in my previous posts, Jason's solution doesn't depend on "GUIDs" at all.
  It's based on a simple idea that is, in hindsight, quite obvious.
---

Previously on this blog:

* ["A metaprogramming puzzle: Overly interoperable libraries"](/blog/2021/05/19/after-you-no-after-you/) (2021-05-19)

* ["Overly interoperable libraries, part 2"](/blog/2021/05/22/after-you-part-two/) (2021-05-22)

Jason Cobb has sent me a proposal that I think meets all the requirements and
therefore qualifies as a complete solution!

Unlike the ideas in my previous posts, Jason's solution doesn't depend on "GUIDs" at all.
It's based on a simple idea that is, in hindsight, quite obvious.

> It's tricky to distinguish "zero" from "one or two or more."
>
> But it's easy to distinguish "exactly one" from "two or more."
>
> So, add one!

Jason's solution to the original puzzle looks like this ([Godbolt](https://godbolt.org/z/hW4jfzG1s)):

    struct tag {};
    struct disable_tag {};

    template<class T, class U>
    disable_tag enable_xor(tag);

    template<class T, class U, class = disable_tag>
    struct is_xor_enabled : std::true_type {};

    template<class T, class U>
    struct is_xor_enabled<T, U, decltype(enable_xor<T, U>(tag{}))> : std::false_type {};

    template<class T, class U, std::enable_if_t<is_xor_enabled<T, U>::value, int> = 0>
    int operator^(T t, U u) {
        return t.value() ^ u.value();
    }

    ~~~

    template<class T, class U,
        // Foo can be xor'ed with anything foolib-compatible.
        std::enable_if_t<std::is_same<T, Foolib::Foo>::value, int> = 0,
        std::enable_if_t<U::foolib_compatible, int> = 0>
    void enable_xor(tag);

    ~~~

    template<class T, class U,
        // Bar can be xor'ed with anything barlib-compatible.
        std::enable_if_t<std::is_same<U, Barlib::Bar>::value, int> = 0,
        std::enable_if_t<T::barlib_compatible, int> = 0>
    void enable_xor(tag);

The overload set for the `decltype`’d call to `enable_xor<T, U>(tag{})` consists
of the initial `disable_tag` overload, plus any of the library-specific overloads
that remain viable after template substitution. If any library cares about this
particular set of arguments `<T, U>`, then the `decltype`’d call to `enable_xor<T, U>(tag{})`
will be ambiguous and `is_xor_enabled<T, U>::value` will be `true`.
If no library cares about `<T, U>`, then the `decltype`’d call will _not_ be
ambiguous — the only viable candidate will be the initial `disable_tag` overload —
and so `is_xor_enabled<T, U>::value` will be `false`.

This solution easily handles my followup post's `Wimpylib` example ([Godbolt](https://godbolt.org/z/3Pe3fPdhj)):

    // Any wimpy-compatible type can be xored with any other type.
    template<class T, class U, std::enable_if_t<T::wimpy_compatible, int> = 0>
    void enable_xor(tag);
    template<class T, class U, std::enable_if_t<U::wimpy_compatible, int> = 0>
    void enable_xor(tag);

Whatever the prize was for solving this puzzle, Jason has won it!

----

The code above uses `struct disable_tag` merely to get a unique return type that
won't collide with the return type of any of the user's library overloads. You
could replace it with something like `int`, no problem.

But `struct tag` cannot be replaced with `int`! If you do that, suddenly the compiler
stops finding the library overloads of `enable_xor` as candidates at all. The reason
is two-phase lookup. When we ask for `decltype(enable_xor<T, U>(tag{}))` in a dependent
context, the compiler considers all of the candidates found by ordinary unqualified lookup
right now (in "phase one"), plus all of the candidates found by ADL at instantiation
time ("phase two"). The compiler does _not_ consider candidates found by ordinary unqualified
lookup in phase two! So it's actually super important that the various library overloads
be placed in a namespace associated with one of `enable_xor`'s function arguments.

`int` is a primitive type: it has no associated namespaces. `struct tag`, on the other hand,
has one associated namespace — the global namespace. So, because we use `struct tag` as the
function argument type in our `decltype`’d `enable_xor<T,U>(tag{})`, ADL is able to find all
these `enable_xor` overloads that the various libraries have dumped into the global namespace.

----

I feel like I've seen this "add one" trick somewhere before. I'm not sure if this is exactly
where I saw it, but at least it occurs to me that you can use this trick to determine whether
a given type has any member named `m`, even when that member is non-public.
[Godbolt](https://godbolt.org/z/9xf195P67):

    template<class T, class = void>
    constexpr bool naive_has_m = false;
    template<class T>
    constexpr bool naive_has_m<T, std::void_t<typename T::m>> = true;

    class YesButPrivate { using m = int; };

`naive_has_m<YesButPrivate>` gives `false`, because the name `T::m` is private. (Except on GCC,
where it's currently a hard error: that's a bug.) So, _add one..._

    struct Sibling { using m = void; };
    template<class T> struct Helper : T, Sibling {};
    template<class T, class = void>
    constexpr bool smart_has_m = true;
    template<class T>
    constexpr bool smart_has_m<T, typename Helper<T>::m> = false;

`smart_has_m<YesButPrivate>` gives `true`, because name lookup on `Helper<T>::m` results
in ambiguity. Only when `T::m` does not exist at all are we able to look up
`typename Helper<T>::m` unambiguously. (In that case of course it will be public and `void`.)
