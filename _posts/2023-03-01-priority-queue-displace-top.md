---
layout: post
title: "Making `priority_queue` and `flat_set` work with move-only types"
date: 2023-03-01 00:01:00 +0000
tags:
  library-design
  llvm
  move-semantics
  relocatability
  stl-classic
---

Using a `std::priority_queue` with move-only elements is pretty clunky.
(This has been known for a long time: [Usenet](https://comp.lang.cpp.moderated.narkive.com/jfbCYoFr/broken-interaction-between-std-priority-queue-and-move-only-types),
[SO](https://stackoverflow.com/questions/20149471/move-out-element-of-std-priority-queue-in-c11)...)

    struct T {
        std::unique_ptr<int> p_;
        explicit T(int i) : p_(std::make_unique<int>(i)) {}
        int value() const { return *p_; }
        friend auto operator<=>(const T& a, const T& b) { return a.value() <=> b.value(); }
    };

    std::priority_queue<T> pq;
    pq.emplace(1);
    pq.emplace(2);
    pq.emplace(3);
    assert(pq.top().value() == 3);

The trouble hits when you want to extract the top value
for processing outside the queue ([Godbolt](https://godbolt.org/z/dqvMhdYK8)):

    T t = pq.pop();  // nope, returns void
    T t = pq.top();  // nope, returns const T&, T isn't copyable

See, `pq` can't allow you to access `pq.top()` as a mutable `T&`, because that would allow you
to change its value, which might make it no longer the biggest element — breaking the priority
queue's invariant. It's the same reason you're not allowed to get mutable access to the elements
of a sorted `set` (or, in C++23, `flat_set`).

    std::set<T> s;
    s.emplace(1);
    s.emplace(2);
    s.emplace(3);
    auto it = s.begin();
    assert(it->value() == 1);
    T t = *it;          // nope, returns const T&, T isn't copyable
    T t = s.erase(it);  // nope, returns iterator

What we need is a method that can move-from the top element _and remove it
from the container_ "in one breath," so that the container's invariant
is preserved. This "extracting erase" operation might be considered the
opposite of the "constructing insert" `emplace`, so the name `displace`
seems plausible to me.

    T t = pq.displace_top();  // make pq one element shorter
    T t = s.displace(it);     // erase(it), returning move(*it)

I've just implemented this in my fork of libc++, so you can try it out
[on Godbolt](https://godbolt.org/z/qd55YGY78) (once the nightly build runs, anyway).

## `set` can kind of already do this

`set` and `unordered_set` actually do allow you to displace elements today,
via the node-handle API. This is of course [very silly-looking](https://godbolt.org/z/TMfbWaaEj),
but it has exactly the correct effect at runtime:

    T t = std::move(s.extract(it).value());

However, this doesn't work for `priority_queue` and `flat_set`, because they're both
based on an underlying sequence container (like a `vector`), rather than a collection
of "nodes" that you can unhook individually at will.

## This doesn't relate much to trivial relocatability

When I started writing this post, I thought relocation would be huge help here!
After all, "move out of a thing, and destroy the source" is the definition of relocation.
But after implementing `set::displace`, `unordered_set::displace`,
`flat_set::displace`, and `priority_queue::displace_top`, I think
trivial relocatability would help `displace` only when the container
is low-level enough to manage the lifetimes of all its elements manually
(without indirection). For example, `vector::displace` _could_ be implemented as:

    T displace(const_iterator pos) {
        T *p = const_cast<value_type*>(std::addressof(*pos));
        if (std::is_nothrow_relocatable_v<T>) {
            T t = std::relocate(p);
            std::uninitialized_relocate(p + 1, end_--, p);
            return t;
        } else {
            T t = std::move(*p);
            std::copy(p + 1, end_--, p);
            std::destroy_at(end_);
            return t;
        }
    }

but this much simpler implementation costs only a single
extra move/destroy cycle on `*p`:

    T displace(const_iterator pos) {
        T *p = const_cast<value_type*>(std::addressof(*pos));
        T t = std::move(*p);
        erase(pos);
        return t;
    }

## This distantly relates to `priority_queue::replace_top`

When I started writing this post, I thought that perhaps `priority_queue::displace`
could be implemented as "relocate out of the first element; relocate upward its largest
child; relocate upward _its_ largest child, etc." in the same way that today we
implement `pop_heap` as "move out of the first element; move-assign upward its largest child, etc."

> "Wait, I thought `pop_heap` started by swapping a leaf element to the top and sifting it down!"
> This more efficient algorithm is known as ["heapsort with bounce"](https://en.wikipedia.org/wiki/Heapsort#Bottom-up_heapsort)
> and [was implemented in libc++ 15](https://github.com/llvm/llvm-project/commit/79d08e398c17e83b118b837ab0b52107fd294c3e).

But we can't actually do that. The elements in the `priority_queue` aren't owned by the `priority_queue`;
they're owned by the underlying `vector`. So the `priority_queue` can't just relocate-out-of them (that is,
end their lifetimes) willy-nilly. Neither could an STL algorithm analogous to `pop_heap`.

There are two actually safe ways to implement `priority_queue::displace_top`:

    // Implementation #1: swap, sift down, displace
    std::pop_heap(c.begin(), c.end(), comp);
    return c.displace_back();

    // Implementation #2: move, displace, move-assign, destroy, sift down
    value_type v = std::exchange(c.front(), c.displace_back());
    std::poke_heap(c.begin(), c.end(), comp);
    return v;

The `poke_heap` algorithm used in Implementation #2 is the same algorithm we need for `priority_queue::replace_top`!
See ["`priority_queue` is missing an operation"](/blog/2018/04/27/pq-replace-top/) (2018-04-27).
Unfortunately, as you can see from the comments above, Implementation #1 gives better codegen anyway.
It might help to have a relocation-based version of `std::exchange`, but C++ isn't great at "functions
that take recipes for prvalues as arguments."
See ["Superconstructing super elider, round 2"](/blog/2018/05/17/super-elider-round-2/) (2018-05-17).

    // Implementation #3: relocate, displace, sift down
    value_type v = std::relocating_exchange(c.front(), []{ return c.displace_back(); });
    std::poke_heap(c.begin(), c.end(), comp);
    return v;
