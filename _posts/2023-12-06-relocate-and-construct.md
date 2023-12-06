---
layout: post
title: "Moving via relocate-and-construct"
date: 2023-12-06 00:01:00 +0000
tags:
  relocatability
  sufficiently-smart-compiler
excerpt: |
  A slow-moving StackOverflow [discussion thread](https://stackoverflow.com/questions/59422888/can-we-detect-trivial-relocatability-in-c17/)
  alerted me to this interesting application of P1144 `std::relocate`
  ([Godbolt](https://godbolt.org/z/jK81xbj9Y)):
---

A slow-moving StackOverflow [discussion thread](https://stackoverflow.com/questions/59422888/can-we-detect-trivial-relocatability-in-c17/)
alerted me to this interesting application of P1144 `std::relocate`
([Godbolt](https://godbolt.org/z/jK81xbj9Y)):

On libc++ and MSVC, `std::array<std::string, 1000>` is a trivially relocatable type;
but it's not trivially move-constructible, because `string`'s move-constructor has to null
out the source object. Moving a single string is tantamount to a `memcpy`
plus a `memset` (to null out the source). So, move-constructing an `array<string, 1000>` is tantamount
to a 1000-times-larger `memcpy` plus a 1000-times-larger `memset`. But no mainstream compiler
is smart enough to [fission](https://en.wikipedia.org/wiki/Loop_fission_and_fusion) the loop body in that way.

[P1144 `std::relocate`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r9.html#wording-relocate)
gives us a name for `array<string>`'s operation that is tantamount to `memcpy`; and we already have a name
("default constructor") for `array<string>`'s operation that is tantamount to `memset`.
So — here's the cool part — we can help our insufficiently-smart compiler see the right way
to factor this code by writing the move-construction as a `relocate` plus a placement-`new`.

    using A = std::array<std::string, 1000>;

    A *move_to_heap(A& a) {
      // Compiler fails to fission the loop
      return new A(std::move(a));
    }

    A *move_to_heap(A& a) {
      // Compiler generates memcpy + memset
      auto *p = new A(std::relocate(&a));
      ::new (&a) A;
      return p;
    }

Of course this trick isn't general-purpose. It does work for non-trivially relocatable types
(for which `std::relocate` rightly won't generate a `memcpy`); but it doesn't compile for types that aren't default-constructible,
nor does it work for types whose default constructor has unwanted side effects, or whose moved-from state is distinguishable
from their default-constructed state. (As usual, `std::pmr::string` is an example.) Therefore
I wouldn't expect to see this trick in generic code.

But this _is_ a neat application of the P1144 high-level primitives (is that an oxymoron?)
to help the compiler see how to generate optimal code, without throwing up our hands and descending
into undefined-behavior-land. I can't think of a "standard-compliant" way to get the `memcpy`+`memset`
codegen in today's C++ — but with P1144's vocabulary, it's straightforward.
