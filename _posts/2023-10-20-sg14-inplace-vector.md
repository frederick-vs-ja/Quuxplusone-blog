---
layout: post
title: "`Quuxplusone/SG14` now has `inplace_vector`"
date: 2023-10-20 00:01:00 +0000
tags:
  allocators
  library-design
  proposal
  relocatability
  sg14
  standard-library-trivia
---

{% raw %}
This week I implemented [P0843R9 "`inplace_vector`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p0843r9.html)
in the SG14 algorithms-and-data-structures repository
[`Quuxplusone/SG14`](https://github.com/Quuxplusone/SG14/#in-place-vector-future--c17).
This is the same container that Boost.Containers calls
[`static_vector`](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/container/static_vector.html).
Personally, I prefer the name `fixed_capacity_vector` — see
["`inplace_foo` versus `fixed_capacity_foo`"](/blog/2018/06/18/inplace-vs-fixed-capacity/) (2018-06-18) —
but I think WG21 has litigated that name to death already, so `inplace_vector` it is!

`sg14::inplace_vector` supports trivial relocatability out of the box
(as long as you use [my fork of Clang](https://p1144.godbolt.org/z/8G7s97x4h) or another
compiler supporting P1144 trivial relocation). For example,

    sg14::inplace_vector<T, 4> v, w;
    v = std::move(w);

is trivial when `T` is trivial; and when `T` is trivially relocatable, it compiles to the
equivalent of

    if (w.size() < v.size()) {
      std::move(w.data(), w.data() + w.size(), v.data());
      std::destroy(v.data() + w.size(), v.data() + v.size());
      v.size_ = w.size_;
    } else {
      std::move(w.data(), w.data() + v.size(), v.data());
      std::uninitialized_relocate(w.data() + v.size(), w.data() + w.size(), v.data() + v.size());
      std::swap(v.size_, w.size_);
    }

The `else` branch says, "Assign the first `v.size()` elements; then relocate the remaining
elements from `w` into `v`, thus simultaneously decreasing `w`'s size and increasing `v`'s size."

> Notice that the `else` branch is exception-safe only when `uninitialized_relocate`
> is guaranteed not to throw. If it throws, then we need to set `w.size_ = v.size_`
> before propagating the exception, because `uninitialized_relocate` will have cleaned
> up after itself by destroying all of the objects in both the source and target ranges.
> That cleanup behavior is common to all the `uninitialized_foo` algorithms; see
> [[specialized.algorithms.general]/2](https://eel.is/c++draft/specialized.algorithms#general-2).
>
> This suggests that [P1144](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r9.html#wording-uninitialized.relocate)
> ought to do at least one of three things: (1) Guarantee that
> `std::uninitialized_relocate` throws nothing for trivially relocatable types;
> and/or (2) Restore [R7](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r7.html#wording-meta.unary.prop)'s
> `is_nothrow_relocatable` type-trait with behavior tied specifically to the throwingness
> of `uninitialized_relocate` and company; and/or (3) Give `uninitialized_relocate` a conditional
> noexcept-specification (which is [a bad idea](/blog/2018/06/12/attribute-noexcept-verify/)).
> I'm leaning toward (1), which is spiritually close to
> [P2786](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2786r3.pdf)'s
> proposal of a dedicated entrypoint `std::trivially_relocate` that would be guaranteed
> nothrow but (in return) only conditionally callable.

---

The only defect I intentionally corrected in P0843R9 — besides that P0843R9 marks a lot
of things `constexpr` that can't be constexpr-evaluated for subtle abstract-machine reasons — is
that it was missing an overloaded assignment operator of the form

    Vec& operator=(initializer_list<value_type>);

Every existing STL container has one of these. For ordinary containers it's fairly
redundant, because even if it didn't exist, overload resolution would simply give you
a pair of calls to

    Vec(initializer_list<value_type>); // implicit conversion
    Vec& operator=(Vec&&);  // move-assignment

and the move-assignment is generally cheap.
But when move-assignment _isn't_ cheap — as for `inplace_vector` —
the `initializer_list` overload cuts out the middleman.

In fact, a custom allocator with POCMA `false` can not only be expensive to move-assign,
but can even give you different runtime behavior depending on whether a temporary container
is created or not! [Godbolt](https://godbolt.org/z/MGW6G56xn):

    std::pmr::monotonic_buffer_resource mr;
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    auto v = std::pmr::vector<int>(&mr);
    v = {1,2,3};

That last line is okay as written, but if it were `v = {{1,2,3}}` then we'd try to allocate
from the default arena (which is null) and crash.

> Of course it's possible to blow your whole leg off with `std::pmr`
> in several ways; and `operator=(initializer_list)` predates `std::pmr`
> by two whole C++ versions. I don't know why it was originally
> added to the library in the C++0x timeframe. [N2215](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2215.pdf)
> §10.5 and §11.3 seem related. My guess is that these assignment operators
> were added under the _mistaken_ impression that they would be needed for
> correctness (before the overload-resolution rules for braced-init-lists had
> solidified), and then simply forgotten about. Finally, in C++17, `std::pmr`
> restored a positive reason for them to exist.<br>
> See ["Origins and purposes"](/blog/2023/10/18/origins-and-purposes/) (2023-10-18).

---

If you find a bug or defect in `sg14::inplace_vector`, please report it to me
and/or via the [GitHub issues list](https://github.com/Quuxplusone/SG14/issues).
Pull requests welcome!
{% endraw %}
