---
layout: post
title: 'A "pick two" triangle for `std::vector`'
date: 2022-09-30 00:01:00 +0000
tags:
  exception-handling
  move-semantics
  stl-classic
excerpt: |
  While writing ["What is the 'vector pessimization'?"](/blog/2022/08/26/vector-pessimization/) (2022-08-26),
  I realized that there was a mildly interesting puzzle here. Recall that the problem with `std::vector`
  in C++11 was that it wanted to use move instead of copy; but if a move fails then your strong exception
  guarantee is downgraded to the basic exception guarantee. The C++11 STL's `std::vector` solved this
  problem via the "vector pessimization": it always preserves the strong guarantee, but at the cost of
  sometimes copying instead of moving.

  But from the programmer's point of view, this is a "pick two" triangle, like
  the [CAP theorem](https://en.wikipedia.org/wiki/CAP_theorem) or
  [Arrow's paradox](https://en.wikipedia.org/wiki/Arrow%27s_impossibility_theorem).
  You can have a vector that reallocates by moving instead of copying; or you can have
  a vector that propagates exceptions thrown during reallocation; or you can have a vector
  that gives the strong exception guarantee (instead of merely the basic guarantee); in fact
  you can have any two of these at once; but you can't have all three!
---

While writing ["What is the 'vector pessimization'?"](/blog/2022/08/26/vector-pessimization/) (2022-08-26),
I realized that there was a mildly interesting puzzle here. Recall that the problem with `std::vector`
in C++11 was that it wanted to use move instead of copy; but if a move fails then your strong exception
guarantee is downgraded to the basic exception guarantee. The C++11 STL's `std::vector` solved this
problem via the "vector pessimization": it always preserves the strong guarantee, but at the cost of
sometimes copying instead of moving.

But from the programmer's point of view, this is a "pick two" triangle, like
the [CAP theorem](https://en.wikipedia.org/wiki/CAP_theorem) or
[Arrow's paradox](https://en.wikipedia.org/wiki/Arrow%27s_impossibility_theorem).
You can have a vector that reallocates by moving instead of copying; or you can have
a vector that propagates exceptions thrown during reallocation; or you can have a vector
that gives the strong exception guarantee (instead of merely the basic guarantee); in fact
you can have any two of these at once; but you can't have all three!

Let's look at three programmers' business requirements for code
something like this:

    struct Widget {
        std::list<int> m_ = std::list<int>(1000);
            // MSVC's std::list isn't nothrow movable
    };

    std::vector<Widget> v(10);
    try {
        v.reserve(v.capacity()+1);  // reallocate the buffer
    } catch (...) {
        // is v still in its old state?
    }

**Alice** says: "I want the strong exception guarantee, and I can meaningfully catch `bad_alloc`.
Contrariwise, I don't care about the speed of my reallocation operation." Plain old C++ is perfect
for Alice. Her `Widget` should follow the Rule of Zero. `std::vector` will copy it instead of
moving (since MSVC's `std::list` makes `Widget`'s defaulted move constructor non-noexcept).
She gets the strong guarantee, and can also detect and recover when copying all those lists
runs her out of memory.

**Bob** says: "I want speed, and I want the strong exception guarantee. Contrariwise, I don't
care about catching `bad_alloc` — just crash!" (Incidentally, see
[P1404 "`bad_alloc` is not out-of-memory!"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1404r1.html)
(Andrzej Krzemieński and Tomasz Kamiński, June 2019) for some situations where
Bob's analysis might be inappropriate. Personally, I think Bob is more often right
than wrong, but for our purposes here it doesn't even matter if he's right — just that
he represents one of the sides of our two-out-of-three triangle.) Since Bob will accept
program termination if moving `m_` throws, Bob can just mark
his explicitly defaulted move constructor as `noexcept`:

    struct Widget {
        std::list<int> m_ = std::list<int>(1000);
            // MSVC's std::list isn't nothrow movable

        explicit Widget() = default;
        Widget(Widget&&) noexcept = default;  // !!
        Widget(const Widget&) = default;
        Widget& operator=(Widget&&) = default;
        Widget& operator=(const Widget&) = default;
    };

    std::vector<Widget> v(10);

This causes the compiler to generate a defaulted move constructor
_wrapped in a `noexcept` firewall_. Adding `noexcept` (or `explicit`,
or `virtual`) to a defaulted special member in this way is occasionally
useful, and certainly better than trying to implement the whole member's
body by yourself.

**Charlie** says: "I can meaningfully catch `bad_alloc`, and I want speed.
Contrariwise, I don't care about the strong exception guarantee — if reallocation
causes `Widget`'s move-constructor to throw, then I don't care what state `v`
ends up in!"

Alice and Bob had easy solutions using the standard `std::vector`.
Charlie, though, is out of luck. There is no way (as far as I know)
to achieve an "always-move, basic-guarantee `vector<Widget>`" except
to build it from scratch. As soon as you involve `std::vector` in
your solution, you're locked into the strong exception guarantee,
and will never reach Charlie's side of the two-out-of-three triangle.

Do you know a way to achieve Charlie's aims — exception propagation
from `Widget`'s move constructor, but a `vector<Widget>` that provides
only the basic guarantee? Does there exist a well-tested "`basic_guarantee_vector<T>`"
anywhere out there?
