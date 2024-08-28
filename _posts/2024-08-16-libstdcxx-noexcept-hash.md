---
layout: post
title: "`noexcept` affects libstdc++'s `unordered_set`"
date: 2024-08-16 00:01:00 +0000
tags:
  benchmarks
  data-structures
  exception-handling
  stl-classic
  standard-library-trivia
  triviality
---

The other day I learned a new place where adding or removing `noexcept` can
change the performance of your program: GNU libstdc++'s
hash-based associative containers change the struct layout of their nodes
depending on the noexceptness of your hash function.
This is laid out fairly clearly
[in the docs](https://gcc.gnu.org/onlinedocs/libstdc++/manual/unordered_associative.html);
it's simply bizarre enough that I'd never thought to look for such a thing _in_ the docs!

In C++, a `std::unordered_set` is basically a vector of "buckets," where each bucket is
a linked list of "nodes," and each node stores a single element of the `unordered_set`.
(This makes `unordered_set` a "node-based container"; it supports the full
[C++17 node-handle API](https://en.cppreference.com/w/cpp/container/unordered_set/extract).)
Erasing an element from the `unordered_set` goes something like this:

    template<class T, class Hash, class KeyEqual>
    struct unordered_set {
      struct Node {
        T t_;
      };
      vector<list<Node>> buckets_;
      Hash hash_;
      KeyEqual eq_;

      void erase(const T& key) {
        size_t h = hash_(key);
        size_t bc = buckets_.size();
        auto& bucket = buckets_[h % bc];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
          if (eq_(it->t_, key)) {
            bucket.erase(it);
            return;
          }
        }
      }
    };

(Some details omitted, e.g. the `Allocator` parameter and the return type of `erase`.)

Above, the `for`-loop is iterating over everything in the bucket — that is, every item
`t` such that `hash_(t) % bc == hash_(key) % bc` — and invoking `eq_` for each of
those items. If equivalence is expensive, then maybe we could do better by comparing
hashes first:

        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
          if ((hash_(it->t_) == h) && eq_(it->t_, key)) {
            bucket.erase(it);
            return;
          }
        }

Now we invoke `eq_` for only those items `t` such that `hash_(t) == hash_(key)`, period.
But if `hash_` is _slower_ than `eq_`, that's a pessimization.

## Store a hash per node

We can keep it fast by precomputing `hash_(it->t_)` and storing it in the node directly:

      struct Node {
        T t_;
        size_t h_;
      };

      void erase(const T& key) {
        size_t h = hash_(key);
        size_t bc = buckets_.size();
        auto& bucket = buckets_[h % bc];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
          if ((it->h_ == h) && eq_(it->t_, key)) {
            bucket.erase(it);
            return;
          }
        }
      }

Now we have the best of both worlds, at the piddling cost of 8 bytes per element.
(Sure, in some domains that's a lot! But `unordered_set` is already spending at least that much
on prev/next pointers, not to mention the lower-level bookkeeping associated with each
element's having its own separate heap-allocation. In the grand scheme of `unordered_set`
inefficiencies, another 8 bytes won't be missed.)

Secondly: The libstdc++ docs [go on to say](https://gcc.gnu.org/onlinedocs/libstdc++/manual/unordered_associative.html) that
"`erase` and `swap` operations [...] might need an element's hash code." I'm not sure why they
mention `swap`, but I see how `erase` needs an element's hash code. We're no longer talking about
`erase(const T&)`; now we're talking about `erase(const_iterator)`. That looks something like this:

    void erase(list<Node>::const_iterator it) {
      size_t h = it->h_;
      size_t bc = buckets_.size();
      buckets_[h % bc].erase(it);
    }

Here we need `h` so that we can figure out which bucket to modify.

> "Can't we just hook our `prev` pointer up to our `next` pointer?"
> Well, it turns out that I lied about the data structure being basically a `list`.
> Microsoft's `unordered_set` has bidirectional iterators, but libc++ and libstdc++
> allow only forward iteration. libstdc++ actually
> [has code](https://github.com/gcc-mirror/gcc/blob/cc38bdf/libstdc%2B%2B-v3/include/bits/hashtable.h#L2123-L2130)
> that re-iterates the bucket looking for this node's predecessor. "Isn't that slow?"
> Sure, if you have a high [load factor](https://en.cppreference.com/w/cpp/container/unordered_set/load_factor);
> but the whole point of a hash table is that the load factor _must_ remain low
> as `size()` increases, or else you have a bad time (in more ways than this).

In practice, if we're the first node in our bucket, we also need to update
the `h`’th bucket pointer to point to our successor — the thing slightly automated
above by `buckets_[...].erase(it)`. Even if we had a "prev" pointer, we'd still have
to know which bucket to touch for that part.

Thirdly: Suppose we insert so many elements that we need to expand the `buckets_` vector —
we need to "rehash." Each element `t` is currently part of bucket number `hash_(t) % bc`;
but we'll need to place it into the expanded vector at index `hash_(t) % new_bc`,
which can't be computed without knowing `hash_(t)` itself.

So it seems like we have at least three reasons to want to store those extra 8 bytes
in each node: It can save us O(k) time during `erase(const T&)`; we have to compute it
once anyway during `erase(const_iterator)`; and we have to compute it O(n) times
during rehashing.

Clearly we should just pay those 8 bytes to cache `hash_(t)`, and everyone'll be happy, right?

## But sometimes the hash is trivial

Well, what about `unordered_set<int>`? In that case, the hash function is `std::hash<int>`,
which on libstdc++ [is trivial](https://github.com/gcc-mirror/gcc/blob/cc38bdf093c44918edff819ae6c73d03c726b341/libstdc%2B%2B-v3/include/bits/functional_hash.h#L114-L122):
`hash_(t)` is just a copy of `t` itself. It does seem pretty silly to store that!
So, what we really want is to store the hash by default, and omit it only when `Hash`
is a trivial hash function.

Unfortunately, libstdc++ essentially reverses that logic: By default,
they'll _never_ pay the extra 8 bytes to store the hash. They'll assume that
recomputing the hash is cheap, so that `it->h_` can be replaced with `hash_(it->t_)`.
But there are two things that can make libstdc++ reconsider that decision:

* If `Hash` is on a fixed blacklist of "expensive hash functions," i.e., if
    [`std::__is_fast_hash<Hash>::value`](https://github.com/gcc-mirror/gcc/blob/cc38bdf093c44918edff819ae6c73d03c726b341/libstdc%2B%2B-v3/include/bits/functional_hash.h#L283-L300)
    is false. That tells us that it would be prohibitively expensive to
    recompute `hash_(t)` during rehashing, so we'd better cache it.

* If `Hash::operator()` is non-noexcept. That tells us that when we recompute
    `hash_(t)` during `erase`, it might throw an exception — and the Standard
    guarantees that `erase(const_iterator)` _won't_ throw any exceptions!
    So `erase(const_iterator)` isn't allowed to call the hash function at all,
    in this case.

> <a href="https://eel.is/c++draft/container.requirements#container.reqmts-66.3">[container.reqmts]/66.3</a>
> specifies that "No `erase`, `clear`, `pop_back`, or `pop_front` function throws an exception."
> This is amended by <a href="https://eel.is/c++draft/unord.req.except#1">[unord.req.except]/1</a>,
> which permits `erase(const T& key)` to throw if hashing `key` throws (because we need to hash `key`
> in order to look it up in the first place), but still doesn't permit `erase(const_iterator)` to throw
> for any reason.

The outcome is that libstdc++'s `unordered_set` has performance characteristics
that subtly depend on the true name and noexceptness of the hash function.

* A user-defined `struct H : std::hash<std::string> {}`
    will see smaller allocations and more calls to the hash function, than if you
    had just used `std::hash<std::string>` directly. (Because `std::hash<std::string>`
    is on the blacklist and `H` is not.)

* A user-defined hasher with `size_t operator() const noexcept`
    will see smaller allocations and more calls to the hash function (especially during rehashing).
    One with `size_t operator() const` will see larger allocations and fewer calls to the hash function.

## Benchmark

[This benchmark](/blog/code/2024-08-16-hash-benchmark.cpp) ([on Godbolt, with smaller numbers](https://godbolt.org/z/cP8hscaEW))
demonstrates some of the consequences of libstdc++'s heuristic.
For each of three types, we print the node size and the amount of time it takes to rehash the table 14 times.

|                 | `int*`         | `string`                 | `vector<bool>`                    |
|-----------------|----------------|--------------------------|-----------------------------------|
| `std::hash` is: | trivial (fast) | blacklisted (known slow) | normal (incorrectly assumed fast) |
| non-`noexcept`  | 24 bytes, 15ms | 48 bytes, 275ms          | 56 bytes, 259ms                   |
| `noexcept`      | 16 bytes, 13ms | 40 bytes, 951ms          | 48 bytes, 945ms                   |
| `std::hash`     | 16 bytes, 13ms | 48 bytes, 264ms          | 48 bytes, 946ms                   |

The first column is really cool: it shows that `unordered_set` is smart enough not to cache `hash_(t)`
when `hash_` is the trivial `std::hash<int*>`. Recomputing the hash (a no-op) is actually faster than
loading the cached value from memory!

The second column, bottom cell, illustrates that `unordered_set` correctly pays the 8 bytes to avoid expensively
recomputing `std::hash<string>` over and over. You can replicate that good
behavior by defining your own `YourHash::operator() const`. But woe betide the hapless programmer
who defines `YourHash::operator() const noexcept` — their hash function will be called many more times
than it should, resulting in a massive slowdown (on this workload, anyway).

The third column illustrates the reverse effect: libstdc++ does _not_ recognize that hashing a
`vector<bool>` is expensive, so they save 8 bytes and take a massive slowdown by default.
You can replicate that bad behavior with `YourHash::operator() const noexcept`, or cleverly
define your own `YourHash::operator() const` for a speedup (on this workload).

## Conclusion

I think this whole thing is mostly irrelevant to real programming. [But see the UPDATE below.]
I wouldn't go out of my way to audit
your codebase for hashers that are (or aren't) `noexcept`; if hashing is your bottleneck then your first
step should be to stop using the node-based, pointer-chasing `std::unordered_set` entirely!

Also, I hope that if you're reading this post in a year or two (say, December 2025), these specific
examples won't even reproduce anymore. I hope libstdc++ gets on the ball and eliminates some of
this weirdness. (In short: They should get rid of the blacklist; pay the 8 bytes by default;
introduce a whitelist for _trivial_ hashers specifically; stop checking noexceptness for any reason.)
[But see the UPDATE below.]

But if you must take one sound bite from this post, maybe it's this: **libstdc++ makes it a pessimization
to mark any non-trivial hash function as `noexcept`.** You must put the `noexcept` keyword on non-defaulted move-constructors
(to avoid the [vector pessimization](/blog/2022/08/26/vector-pessimization/)); there are a few other ad-hoc places
you might want it; but you certainly do _not_ want `noexcept` "everywhere," as this surprising libstdc++ behavior
illustrates.

---

UPDATE, 2024-08-27: I originally ended by saying "I hope libstdc++ gets on the ball and eliminates some of
this weirdness," but Lénárd Szolnoki points out that there's no way for them to do that without an ABI break,
and I think he's 100% right. So if you're reading this in December 2025, or even 2035, libstdc++ will probably
still be doing the same silly thing with `noexcept` hashers. Ugh.

On /r/cpp, Martin Leitner-Ankerl took issue with my optimistic claim that
"this whole thing is mostly irrelevant to real programming." Apparently Bitcoin Core was bitten by this issue
in real life. Their problem was the opposite of the one I sketched:
each Bitcoin node stores a huge in-memory `unordered_map` whose key is cheap to hash, but whose hash function,
until 2019, wasn't marked as `noexcept`. So their `unordered_map` consumed an extra 8 bytes per node. Adding the
`noexcept` keyword (in [commit 67d9990](https://github.com/bitcoin/bitcoin/commit/67d99900b0d770038c9c5708553143137b124a6c))
instantly lowered their memory usage on most platforms by 9%. Now, in my defense, Bitcoin has also weighed
the approach I suggested above — "stop using the node-based, pointer-chasing [`std::unordered_map`] entirely" —
in [#22640](https://github.com/bitcoin/bitcoin/pull/22640), and estimated its memory savings at about 20%.
If I understand correctly, they've decided not to go that route just yet, on the principle of "the devil
you know is better than the devil you don't." But watch that space. I dare say if I ran a huge money-related
project whose performance was dominated by a single hash-table data structure, I wouldn't be content
with an off-the-shelf `std::unordered_map`; the first thing I'd do is own that data structure and start
optimizing it!

---

See also:

* ["What is the vector pessimization?"](/blog/2022/08/26/vector-pessimization/) (2022-08-26)
* ["Type-erased `InplaceUniquePrintable` benefits from `noexcept`"](/blog/2022/07/30/type-erased-inplace-printable/) (2022-07-30)
* ["`unordered_multiset`’s API affects its big-O"](/blog/2022/06/23/unordered-multiset-equal-range/) (2022-06-23)
