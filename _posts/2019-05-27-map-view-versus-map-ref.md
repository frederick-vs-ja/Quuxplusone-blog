---
layout: post
title: "`MapView` can be faster than `MapRef`"
date: 2019-05-27 00:01:00 +0000
tags:
  benchmarks
  cppnow
  library-design
  parameter-only-types
---

Chandler Carruth gave [a pretty great lightning talk](https://www.youtube.com/watch?v=kye4aD-KvTU)
at C++Now 2019 on his new ideas for a
`Map` API; that is, the API for a data structure that stores key-value pairs. I disagreed
with some of his intuitions, such as the promiscuous use of overloading — in particular
I recall that some member functions took lambdas and were overloaded based on their
_parameter's_ parameter type! Yuck! But the thing that made it great was this observation:

> Sometimes a `view` type is not just a convenience, but a performance optimization.

Chandler's library provided not only `Map<K,V>`, but also `MapRef<K,V>` and `MapView<K,V>`.
The difference between `MapRef` and `MapView` is mutability. `MapRef` is mutable,
whereas `MapView` is immutable.

A `MapView` is more than just a `ConstMapRef`. The name `ConstMapRef` implies that
_the holder of the reference_ can't modify the map, but maybe someone else can. That is,
a `ConstMapRef` behaves pretty much just like a `const Map&`.
The name `MapView` implies that the _map itself_ can't be modified. That is, a `MapView`
behaves pretty much like an iterator-pair, or a reference to some _element_ of the map.
If I've got a `MapView` into your map, and then you modify that
map by inserting or deleting elements, your action invalidates my view.
Accessing via an invalid view produces undefined behavior.

Suppose we have a `Map` type like Chandler's. The elements of the map are stored in
contiguous memory (unlike either `std::map` or `std::unordered_map`) because we like
cache-locality. Therefore, we might as well add a small buffer optimization (SBO).
If the map has few enough elements, we'll just store them all in-line.

But SBO has a cost! Now every time we call `m.lookup(key)`, we have to start by getting
a pointer to the map's contiguous data. We could pull a libstdc++ `std::string` and
store a pointer-to-our-own-buffer, but that would harm our trivial relocatability.
So instead, `m` compares its size to the capacity of the SBO buffer and branches on the
result. It does this comparison and branch on every lookup. (Modulo the as-if rule,
of course.)

When we create a `MapView`, on the other hand, we know that the underlying map will never
change size, and so its contiguous data will remain in the same place
on every lookup. We can cache the `Map`'s data pointer as a member variable of our `MapView` object.
(Notice that our `MapView` remains trivially relocatable.)  This means that when we use
a `MapView`, we _don't_ perform a compare-and-branch on every `lookup`! Lookups in a `MapView`
can be faster than lookups in a fundamentally-mutable `Map`.

----

I wrote a quick benchmark to test this intuition with respect to `const std::string&` versus
C++17 `std::string_view`. [Here are the results](http://quick-bench.com/LMTAbv7DsLVWC14BjdYvVwR0w_A)
([backup](/blog/code/2019-05-27-string-view-benchmark.cpp)). Superficially, they support the
intuition above.

Iterating over the elements of a string via

    int n = s.size();
    for (int i=0; i < n; ++i) {
        result += s[i];
    }

takes noticeably longer when `s` is a `const std::string&` than when `s` is a `std::string_view`.

However, I think it would be wrong in this case to blame the compare-and-branch used (on libc++)
to fetch the data pointer; the assembly shows us that the compiler is sufficiently smart to
see that the string is not modified and thus to hoist the compare-and-branch out of the inner loop.
The other weird thing going on there is that using a ranged `for` loop actually hurts our performance
a lot. I haven't figured out what Clang is doing there. ([Godbolt.](https://godbolt.org/z/jwLxja))

Where Chandler's differentiation of `const Map&` from `MapView` could _really_ save us a lot of time,
even on a trivial benchmark like this one, is if `Map::lookup` and `MapView::lookup` are implemented
in a different object file. That would prevent the compiler from hoisting the compare-and-branch
as it has done in this simple case.

Conclusion: You can sometimes think of a `view` type as a particularly ergonomic form of manual
[loop-invariant code motion](https://en.wikipedia.org/wiki/Loop-invariant_code_motion).
