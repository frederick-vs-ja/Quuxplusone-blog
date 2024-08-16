---
layout: post
title: "`unordered_multiset`'s API affects its big-O"
date: 2022-06-23 00:01:00 +0000
tags:
  benchmarks
  data-structures
  library-design
  standard-library-trivia
  stl-classic
---

An STL-style container's performance can be dramatically affected by
minor changes to the underlying data structure's invariants, which
in turn can be dramatically constrained by the container's chosen API.
Since [the Boost.Unordered performance project](https://bannalia.blogspot.com/2022/06/advancing-state-of-art-for.html)
has put `std::unordered_foo` back in the spotlight, I thought this
would be a good week to talk about my favorite little-known STL trivia
tidbit: `std::unordered_multiset`'s decision to support `.equal_range`
dramatically affects its performance!


## Background

First some background on the original `std::multiset`, which was part of C++98's STL.
Both `set` and `multiset` are represented in memory as a sorted binary search tree
(specifically, on all implementations I'm aware of, it's a
[red-black tree](https://en.wikipedia.org/wiki/Red%E2%80%93black_tree));
the only difference is that `multiset` is allowed to contain duplicates.

    std::set<int> s = {33,11,44,11,55,99,33};
    std::multiset<int> ms = {33,11,44,11,55,99,33};
    assert(std::ranges::equal(s, std::array{11,33,44,55,99}));
    assert(std::ranges::equal(ms, std::array{11,11,33,33,44,55,99}));

Because `set` and `multiset` are stored in sorted order, they have the "special skills"
`.lower_bound(key)`, `.upper_bound(key)`, and `.equal_range(key)`:

    auto [lo, hi] = ms.equal_range(33);
    assert(std::distance(lo, hi) == 2);
    assert(std::count(lo, hi, 33) == 2);
    assert(lo == ms.lower_bound(33));
    assert(hi == ms.upper_bound(33));

When C++11 added `std::unordered_set` and `std::unordered_multiset`, part of the idea
was that (API-wise) they should be drop-in replacements for the tree-based containers.
The only difference is that each unordered container is represented in memory as a
hash table: an array of "buckets," each bucket being a linked list of elements with
the same hash modulo `bucket_count()`. Since the order of the elements depends on
the order of the buckets, and the order of the buckets depends on _hashing_,
not _less-than_, the elements in an unordered container aren't intrinsically stored
in sorted order. On libc++, for example, I see this:

    std::unordered_set<int> us = {33,11,44,11,55,99,33};
    std::unordered_multiset<int> ums = {33,11,44,11,55,99,33};
    assert(std::ranges::equal(us, std::array{55,99,44,11,33}));
    assert(std::ranges::equal(ums, std::array{55,44,33,33,11,11,99}));

And on libstdc++:

    assert(std::ranges::equal(us, std::array{99,55,44,11,33}));
    assert(std::ranges::equal(ums, std::array{99,55,44,11,11,33,33}));

Because `unordered_set` and `unordered_multiset` aren't stored sorted, they
can't possibly have `.lower_bound` and `.upper_bound` methods. But notice that
the unordered containers _do_ provide `.equal_range` as a drop-in replacement
for the tree-based containers' `.equal_range`!

    auto [lo, hi] = ums.equal_range(33);
    assert(std::distance(lo, hi) == 2);
    assert(std::count(lo, hi, 33) == 2);

The existence of `std::unordered_multiset::equal_range` implies that
every standards-conforming implementation of `std::unordered_multiset` must store
duplicate elements adjacent to each other. No conforming implementation
can ever store the sequence

    assert(std::ranges::equal(ums, std::array{55,44,33,11,11,33,99}));

because then the range denoted by the iterator-pair `ums.equal_range(33)`
would contain some elements that were not actually duplicates of `33`.
(By the way, when I say "`a` is a duplicate of `b`," I mean `ums.key_eq()(a,b)`;
in general this is a stronger condition than simply _hashing_ to the same value,
but may be weaker than `a == b` if the container uses a custom comparator.)

> Notice that `ums.equal_range(33)` is well-defined, but `std::equal_range(ums.begin(), ums.end(), 33)`
> would have undefined behavior, because the `std::equal_range` algorithm requires
> as a precondition that the sequence be sorted.


## Consequences of the `equal_range` invariant

For `unordered_multiset`, the decision to provide `.equal_range` has some interesting
consequences. We basically had two possible bookkeeping designs _a priori_:

- Store duplicates adjacent to each other. `equal_range` is possible. `insert` is $$O(n)$$
    (because it must preserve the invariant). `count` and `erase(key)` are relatively faster.

- Store duplicates willy-nilly. `equal_range` is impossible. `insert` is $$O(1)$$.
    `count` and `erase(key)` are relatively slower (because they cannot assume that all the
    duplicates are adjacent).

The Standard, by mandating that `equal_range` must be part of the container's _API_, ends up
incidentally mandating a low-level invariant on the _data structure_, which in turn affects the _performance_
of `insert`, `erase`, and `count`!

> The Standard mandates some other details in more straightforward ways; for example, the obscure
> [`.max_load_factor()`](https://en.cppreference.com/w/cpp/container/unordered_multiset/max_load_factor)
> API, or the [`.bucket_count()`](https://en.cppreference.com/w/cpp/container/unordered_multiset/bucket_count) getter.
>
> Joaquín López Muñoz points out that since unique-key associative containers like `set` provide `.find(key)`,
> it only stands to reason that associative containers that can contain duplicate keys, like `multiset`,
> ought to provide something along the lines of `.find_all_duplicates(key)`. The Standard's
> `unordered_multiset::equal_range(key)` fills this ecological niche in the API, but one could
> imagine an alternative specification compatible with willy-nilly bookkeeping, such as a member function
> that returns a pair of `duplicate_iterator`s with an implementation akin to C++20's
> [`filter_view::iterator`](https://en.cppreference.com/w/cpp/ranges/filter_view/iterator).
> Peter Dimov further points out that `duplicate_iterator` could have different invalidation guarantees;
> for example, we might choose to guarantee that these iterators are never invalidated by the
> insertion or erasure of some unrelated key.

Just for kicks, I modified libc++'s `unordered_multiset` to eliminate `equal_range`
and make the algorithmic changes to `insert`, `count`, and `erase`. ([Here's the patch.](https://github.com/Quuxplusone/llvm-project/commit/unordered-multiset-quick-insert))
Then, I wrote a few quick and dirty benchmarks. You can see the actual benchmark code
[here](/blog/code/2022-06-23-unordered-multiset-benchmark.cpp),
but here's a high-level idea of what each benchmark was testing:

    // inserts (ctor)
    std::unordered_multiset<int> mm(g_data.begin(), g_data.end());

    // insert calls
    for (int i : g_data) mm.insert(i);

    // erase calls
    for (int i : g_unique_values) mm.erase(i);

    // counts (all found)
    for (int i : g_unique_values) mm.count(i);

    // counts (mostly absent)
    for (int i : g_unique_values) mm.count(i + 1);

Here are the benchmark results on a workload of 370,000 ints. First I used
a flat distribution — 37 copies each of 10,000 randomly distributed values.
Then, I ran the same operations again with a more
[Zipf-like](https://en.wikipedia.org/wiki/Zipf%27s_law)
distribution, where the top 10 values appear 1000 times each
and the bottom 9000 values appear only once each.

| Benchmark description           | Before (Flat) | After (Flat) | Before (Zipf) | After (Zipf) |
|---------------------------------|---------------|--------------|---------------|--------------|
| 370,000 inserts (ctor)          |         394ms |         73ms |          68ms |        4.7ms |
| 370,000 `insert` calls          |         382ms |         53ms |          67ms |        4.1ms |
| 10,000 `erase` calls            |         107ms |        113ms |         5.4ms |        5.6ms |
| 10,000 `count`s (all found)     |          30ms |         25ms |         0.6ms |        0.9ms |
| 10,000 `count`s (mostly absent) |         0.3ms |        0.3ms |         0.3ms |        0.3ms |

Scale up the amount of data by a factor of ten, to 3,700,000 inserts,
and the differences become wildly exaggerated.
Remember that our patch doesn't just make `insert` 5x or 10x faster; it makes
`insert` $$O(n)$$ times faster!

| Benchmark description            | Before (Flat) | After (Flat) | Before (Zipf) | After (Zipf) |
|----------------------------------|---------------|--------------|---------------|--------------|
| 3,700,000 inserts (ctor)         |        8735ms |       1828ms |       21345ms |        475ms |
| 3,700,000 `insert` calls         |        8404ms |       1935ms |       22528ms |        408ms |
| 100,000 `erase` calls            |        1782ms |       1799ms |         504ms |        575ms |
| 100,000 `count`s (all found)     |         413ms |        378ms |          80ms |        109ms |
| 100,000 `count`s (mostly absent) |         7.8ms |        9.0ms |         7.9ms |        9.8ms |

The macro trends are as expected: a massive $$O(n)$$–to–$$O(1)$$ speedup for
`insert`, and a constant-factor slowdown for `count`. The results for `erase`
and `count` are surprisingly equivocal. The only explanations I can think of
are that noise in the non-$$O(n)$$ part masked the signal;
or that my new implementations happened to make the optimizer happier than libc++'s
original implementations (for example, by using `nd = nd->next` instead of `++it`
to walk the bucket), and this affected the constant factor enough to cancel out
some of the theoretical advantage. In the case of `count` on the flat distribution,
we even get a reproducible speedup; I can't account for this in any way except to
blame the optimizer.


## Conclusions

The most important takeaway here is that changes to a container's API
can have dramatic and surprising knock-on effects to the underlying
data structure and its performance. I'm trying to distinguish here between on the one hand
the _container and its API_ (`unordered_multiset`, `equal_range`) and on the
other hand the _data structure and its invariants_ (the hash table of linked lists,
the invariant that all duplicates are stored sequentially). In theory, it's possible
to imagine implementing the same container API using different underlying data structures.
But in practice, type authors have to be very careful that when their choice of API
constrains the data structure underneath, those constraints aren't unnatural or unduly
pessimizing.

> For example, above I mentioned that we might provide a `.find_all_duplicates(key)`
> that returns a pair of `duplicate_iterator`s, which we might choose to guarantee
> are never invalidated by the insertion of some unrelated key. That guarantee is an
> API decision that constrains our implementation of `insert`! Now, when `insert` rehashes
> the container, the relative order of each set of duplicates must be preserved;
> otherwise someone iterating over a set of duplicates could enter an infinite loop.
> If we choose an API that doesn't give that guarantee about invalidation, then we
> have more choices about how to implement rehashing.

In the specific case of `unordered_multiset`, I won't even go so far as to say that
C++11 necessarily made the wrong choice. Sure, we could have made `unordered_multiset::insert`
arbitrarily faster, but only at the cost of a constant-factor slowdown to `unordered_multiset::count` —
and so C++11 made a defensible choice from the point of view of a program whose
workload requires many `count`s in a static container.

Much more importantly, C++11 made the right choice from the point of view of a programmer
who already uses the `equal_range` method of `std::multiset` today and wants to upgrade
to `std::unordered_multiset` tomorrow without rewriting too much of their code.

But did C++11 necessarily make the _right_ choice here — do such workloads and programmers
exist in sufficient numbers to justify the performance cost to _other_ programmers?
I wouldn't go so far as to say that, either!

---

See also:

* ["`noexcept` affects libstdc++'s `unordered_set`"](/blog/2024/08/16/libstdcxx-noexcept-hash/) (2024-08-16)
