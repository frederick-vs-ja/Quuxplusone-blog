---
layout: post
title: "Fetishizing class invariants"
date: 2019-02-24 00:01:00 +0000
tags:
  flat-containers
  kona-2019
  library-design
  metaprogramming
  standard-library-trivia
---

What should be the behavior of [the following code](https://wandbox.org/permlink/0rrGQqWBsRev7Z0G)?

    bool danger = true;
    struct RexDangerLess {
        bool operator()(int a, int b) const {
            if (danger && (a == 1 || b == 1)) throw "oops";
            return (a > b);
        }
    };

    int main() {
        std::priority_queue<int, std::vector<int>, RexDangerLess> pq;
        for (int x : {2,3,4,5,6,7,1,3,4,5,6,7}) {
            try { pq.push(x); } catch (...) {}
        }
        danger = false;
        while (!pq.empty()) {
            printf("%d ", pq.top());
            pq.pop();
        }
    }

If you're familiar with [`priority_queue`](/blog/2018/04/27/pq-replace-top/),
you'll know that it keeps its elements in max-heap order. `RexDangerLess` behaves mostly like `std::greater<int>`,
so we'd naturally expect to see the priority queue's elements print out in sorted order:

    1 2 3 3 4 4 5 5 6 6 7 7

But what we actually see is

    2 3 3 4 4 1 5 5 6 6 7 7

Or consider [this code](https://wandbox.org/permlink/UbGNweTnGEfxHf2u):

    struct DJDangerFunc : std::function<bool(int, int)> {
        using std::function<bool(int, int)>::function;
        DJDangerFunc(const DJDangerFunc&) = default;
        DJDangerFunc& operator=(const DJDangerFunc&) { throw "oops"; }
    };

    int main() {
        std::priority_queue<int, std::vector<int>, DJDangerFunc> pq(std::less<>{});
        std::priority_queue<int, std::vector<int>, DJDangerFunc> pq2(std::greater<>{});
        for (int x : {1,2,3,4,5}) {
            pq2.push(x);
        }
        try { pq = pq2; } catch (...) {}
        pq.push(6);
        while (!pq.empty()) {
            printf("%d ", pq.top());
            pq.pop();
        }
    }

On libstdc++, this prints

    6 2 5 4 3 1

On libc++, it prints

    6 3 5 4 2 1

What happened? Well, when certain user-supplied components of `priority_queue` throw exceptions, `priority_queue` rightly
*abandons its class invariant.* It is not one hundred percent true that a `priority_queue` is always kept in max-heap order.
It preserves max-heap order only when its user-supplied pieces are well enough behaved.

----

This week in Kona, LWG spent a fair bit of time discussing
[P0429R6 "A Standard `flat_map`"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0429r6.pdf),
and kind of went down a rabbit-hole on `flat_map`'s class invariants. See, `flat_map` has a _lot_ of invariants.
It stores two vectors (one of keys and one of values), and a user-supplied comparator similar to `priority_queue`'s.
Its class invariants include:

- The two vectors are always of the same size.

- The "keys" vector is always sorted in terms of the comparator.

- Each key's position always matches the position of its corresponding value.

How do we perform an `insert(k, v)` operation on a `flat_map`? Well, if we start by emplacing the key at the end of the keys vector,
then we temporarily break the first two invariants — so, if emplacing at the end of the _values_ vector throws an exception, we must
do extra work to restore those invariants. On the other hand, if we start by emplacing the key at its proper place in the keys vector
(so as to avoid breaking the second invariant), then we temporarily break the first and third invariants, and again must do extra work
to restore them if an exception is thrown.

Worse, if we emplace at the end and then sort the vector to get it back into sorted order, then a throwing comparator
such as `RexDangerLess` can *really* mess us up! If the comparator is allowed to throw, then reliably sorting the vector becomes
impossible — if an exception is thrown from `sort`, then we know we've broken the second invariant, and likely the third as well.

Similar problems crop up in `flat_map::swap`, move-assignment, copy-assignment (as we saw with `priority_queue` in the `DJDangerFunc`
example), and even more places. The `insert(initializer_list)` method is a particularly difficult case to make "exception-safe,"
if you care about preserving `flat_map`'s class invariants. And that's even after LWG decided to require
`is_nothrow_swappable_v<KeyContainer>` and `is_nothrow_swappable_v<MappedContainer>` (which I think is maybe a bit burdensome
on the programmer-in-the-street who rarely uses the `noexcept` specifier in practice, even if `swap` *is* the number two place
you'd want to use it). There was even discussion in LWG of the idea that if an exception is thrown at a sufficiently inopportune
time, the `flat_map` should `clear()` itself — dropping all your data on the floor — in order to restore its class invariants!

But *should* the implementor of `flat_map` care about preserving these invariants? I say no. We've got precedent (in the form of
`priority_queue`) for a standard container adaptor that lives up to its promise of being a thin wrapper around a simple algorithm.
Users who supply throwing operations to their `flat_map` should expect to get broken just as badly as they're broken today
with `priority_queue`. "Play stupid games, win stupid prizes," as they say.

----

How do we codify this design principle?

I would say that if _any_ container adaptor encounters an exception from a user-provided operation, then the container adaptor should
promise nothing more but to propagate that exception and enter a "mostly invalid" state.  An adaptor in a "mostly invalid" state
should support the following operations: destroy, assign-into, `clear()`, and (in `flat_map`'s case) `replace()` and `extract()`.
Nothing else should be guaranteed — not even `size()`.

In `flat_map`'s case, `size()` returns the size of the keys vector which is
invariably the size of the values vector as well. But in the "mostly invalid" state, that invariant could have been broken, and so
whatever number we return even for `size()` can't be trusted.

Inserting or `find`ing in a `flat_map` delegates to `std::lower_bound` (or in practice, `std::partition_point`).
If the keys vector has become unsorted, then those functions will have undefined behavior. So inserting in a "mostly invalid" `flat_map`
should be just as undefined as pushing or popping in a "mostly invalid" `priority_queue`.

One open question on which I expect experts may disagree: Should `flat_map`'s `extract()` operation (which moves-from its
underlying vectors) put the map into a "mostly invalid" state, or should it ensure the postcondition that after the containers
have been `extract`ed the map is guaranteed `empty()`? Earlier this week I thought the latter (and suggested that the postcondition
should be added to `flat_map`'s spec), but I think I've almost entirely come around to the former.

I think wider recognition and adoption of the "mostly invalid state" principle will make it easier for people (including WG21!)
to write their own container adaptors. It will certainly make the specification of `flat_map` shorter!
