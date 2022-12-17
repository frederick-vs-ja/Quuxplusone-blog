---
layout: post
title: 'A metaprogramming puzzle: Overly interoperable libraries'
date: 2021-05-19 00:01:00 +0000
tags:
  metaprogramming
  cpplang-slack
---

Here's an open problem I posted in the cpplang Slack the other day.
I don't claim that it's a _realistic_ or _practically interesting_ problem,
mind you! But it's a metaprogramming problem that I think ought to have
some clever solution; maybe if I blog it, someone will tell me what
that clever solution is.

Consider [this code](https://godbolt.org/z/7ca6o3EG6) ([backup](/blog/code/2021-05-19-after-you.cpp)).
We have three interacting "libraries" here:

- In the global namespace, `operator^` gets defined for any pair of types
    `T`, `U` such that `enable_xor<T,U>::value` is true. Notice that the
    definition of `enable_xor<T,U>` influences the _existence_ of
    `operator^(T,U)`, but not its _behavior_.

- In the `Foolib` namespace, `class Foo` enables `enable_xor<Foo, U>`
    for any type `U` such that `U::foolib_compatible` is true.

- In the `Barlib` namespace, `class Bar` enables `enable_xor<T, Bar>`
    for any type `T` such that `T::barlib_compatible` is true.

So far, `operator^(Foo, Bar)` does not exist.

Now `Foolib`'s maintainer makes `Foolib` aware of `Barlib`:
Set `Foo::barlib_compatible` to `true`.
`operator^(Foo, Bar)` starts working. Awesome.

Now, `Barlib`'s maintainer makes `Barlib` aware of `Foolib`:
Set `Bar::foolib_compatible` to `true`.
`operator^(Foo, Bar)` stops working. _Wait, what?_

Well, when both of the classes are aware of each other, we have two
competing (and equally good) partial specializations. When the compiler tries to
instantiate `enable_xor<Foo, Bar>`, it can't decide which partial
specialization to use — it's ambiguous — and so `enable_xor<Foo, Bar>`
becomes ill-formed.

My metaprogramming puzzle is:

> Find a way to fix this, in the general case,
> for _n_ different libraries, any subset of which might be aware of any of
> the others. All of the libraries should use the same mechanism.
> The mechanism may be parameterized by something for uniqueness,
> such as the library's name or GUID.

Here's a possible solution; its downside is that it is super-duper inefficient
in the general case. ([Godbolt](https://godbolt.org/z/sMqv6a5rs); [backup](/blog/code/2021-05-19-after-you-solution.cpp))
The old mechanism, the one that didn't work, was:

    template<class U>
    struct enable_xor<Foolib::Foo, U,
        std::enable_if_t<U::foolib_compatible>>
        : std::true_type {};

We replace that with a [`priority_tag`](/blog/2021/07/09/priority-tag)-based
function overload, where the priority of the tag is some globally unique random integer.

    template<class T, class U,
        std::enable_if_t<std::is_same<T, Foolib::Foo>::value, int> = 0,
        std::enable_if_t<U::foolib_compatible, int> = 0>
    int enable_xor(priority_tag<42>);  // Our GUID is "42"

`operator^` decides whether to exist based on whether the expression
`enable_xor<T,U>(priority_tag<99>{})` is well-formed. If any of our _n_ libraries have
provided an `enable_xor<T,U>` for any `priority_tag<K>` (for `0 <= K <= N`), then
this function call will be well-formed and unambiguous: the highest `K` will win out.

If two libraries pick the same `K`, then we have an ambiguity again. But we can fix that
with big numbers, right? Just make our overload resolution start at `priority_tag<99999>`
instead of `priority_tag<99>`, right?

Unfortunately, no! Compilers don't like to deal with nested template instantiations beyond,
say, 900 levels of nesting; let alone 99999 levels. So in practice this mechanism cannot
support more than 900 different GUIDs. That's not very "globally unique"! I would say that
this mechanism, as presented, _does not scale._

To really "scale," I think a solution would have to support at least 10,000 different GUIDs
(which really means only about 100 coexisting libraries,
because of the [birthday paradox](https://en.wikipedia.org/wiki/Birthday_problem)).
Even better would be a mechanism supporting 2<sup>64</sup> different GUIDs, or even arbitrarily many
(e.g., something based on strings of arbitrary lengths), or even a mechanism that didn't use
GUIDs at all.

Can you solve it?

----

See also:

* ["Overly interoperable libraries, part 2"](/blog/2021/05/22/after-you-part-two/) (2021-05-22)

* ["Overly interoperable libraries, part 3"](/blog/2021/05/26/after-you-solution/) (2021-05-26)
