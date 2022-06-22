---
layout: post
title: '`std::priority_queue` is missing an operation'
date: 2018-04-27 00:02:00 +0000
tags:
  benchmarks
  data-structures
  library-design
  proposal
---

About a year ago, I read this great paper by Melissa O'Neill titled
["The Genuine Sieve of Erathosthenes"](https://www.cs.hmc.edu/~oneill/papers/Sieve-JFP.pdf) (2009).
She explains how most programs (especially those written by beginners, but *also*
those written by experts) that claim to implement a "sieve" are really implementing
a different algorithm, which she names the "unfaithful sieve" — and then goes on to
show that the unfaithful sieve is asymptotically even worse than the naïve "trial division"
algorithm.

> Whereas the original [unfaithful-sieve] algorithm crosses off all multiples of a prime
> at once, we perform these “crossings off” in a lazier way: crossing off just-in-time.
> For this purpose, we will store a table in which, for each prime `p` that we have discovered so
> far, there is an “iterator” holding the next multiple of `p` to cross off. Thus, instead
> of crossing off all the multiples of, say, 17, at once (impossible, since there are
> infinitely many for our limit-free algorithm), we will store the first one (at 17 × 17;
> i.e., 289) in our table of upcoming composite numbers. When we come to consider
> whether 289 is prime, we will check our composites table and discover that it is a
> known composite with 17 as a factor, remove 289 from the table, and insert 306
> (i.e., 289 + 17). In essence, we are storing “iterators” in a table keyed by the current
> value of each iterator.

So naturally I went and implemented O'Neill's "Genuine Sieve" algorithm in C++, and [posted
the resulting code on StackOverflow](https://codereview.stackexchange.com/questions/163435/the-genuine-sieve-of-eratosthenes-in-c14)
to get feedback. Let's pass over the surprisingly negative review comments and skip
straight to what I want to talk about. :)

My C++14 code has two fundamental pieces, both of which match the "iterator" or
"unbounded range" concepts. The first piece is `iotarator`, which generates an unbounded
range of increasing integers:

    template<typename Int>
    class iotarator {
        Int value = 0;
    public:
        explicit iotarator() = default;
        explicit iotarator(Int v) : value(v) {}
        Int operator*() const { return value; }
        iotarator& operator++() { value += 1; return *this; }
        iotarator operator++(int) { auto ret = *this; ++*this; return ret; }

        bool operator==(const iotarator& rhs) const {
            return value == rhs.value;
        }
        bool operator!=(const iotarator& rhs) const { return !(*this == rhs); }
    };

The second piece is `sieverator`, which fits over the end of an `iotarator` and filters
its range according to O'Neill's sieve algorithm.

Then, the program to produce an infinite stream of primes to `stdout` looks like this:

    int main()
    {
        iotarator<__int128_t> iota(2);
        sieverator<__int128_t> sieve(iota);
        for (int p : sieve) {
            std::cout << p << std::endl;
        }
    }

As written, [my original program](/blog/code/2018-04-27-sieve-original.cpp) produced the first 10,000,000 primes
in about **97 seconds.**

    $ time ./a.out | head -10000000 |tail -1
    179424673

    real    1m36.686s
    user    1m38.039s
    sys     0m38.026s

Confronted with the fact that my "genuine sieve" program was *horribly* slow compared to
an expert's quick-and-dirty implementation of the "unfaithful sieve", I (well, first I waited
a year, but then I) started looking at where the bottlenecks actually were in my original code.


## Obvious bottleneck 1: iostreams

Replacing `std::endl` with `'\n'` reduces the
running time from 97 seconds to **71 seconds.** Replacing `std::cout` with `printf("%d\n", p)` reduces
the running time a tiny bit more, to **67 seconds.**


## Obvious bottleneck 2: premature pessimizations

My original code had done at least three things that might be slowing down my benchmark numbers:

- Using `__int128_t` (which should be "wide enough for anyone") instead of `int64_t`
  (which happens to be wide enough for this particular benchmark, and is of a convenient size
  to do multiplications quickly on x86-64).

- Using a weird type-erased iterator pattern in order to make `sieverator<Int>` not depend on
  the type of iterator it's filtering. In reality, I don't think it even makes sense to filter
  anything except an `iotarator`... and anyway, there's nothing wrong with templating
  `sieverator` on the iterator type — `sieverator<Int, Iterator>`. Premature type-erasure
  is definitely an antipattern I should not have fallen into.

- My original `iotarator` took a `step` parameter, which in practice was always set to `1`.
  (I removed this parameter already, in the code I quoted above, but you can see it in
  [my original program](/blog/code/2018-04-27-sieve-original.cpp).) I bet the compiler inlined
  away all the references to this parameter anyway, but just in case, let's remove it.

[After undoing these premature pessimizations](/blog/code/2018-04-27-sieve-unpessimized.cpp), we're down to **50 seconds.**


## Bottleneck 3: `__sift_down`

By the way, if you don't know how a min-heap works, [now would be a good time to learn.](https://en.wikipedia.org/wiki/Binary_heap#Heap_implementation)
This blog post will assume you already know.

At this point, I decided to open up Instruments Time Profiler and look at where the hotspot
*actually* was. ([Amdahl's Law](https://en.wikipedia.org/wiki/Amdahl%27s_law#Relation_to_the_law_of_diminishing_returns)
for the win!) It turned out that we were spending 94% of our time in
`sieverator<__int128>::is_already_crossed_off(__int128)`, which is not
surprising; that's where the real work gets done. `sieverator` looks something like this:

    template<class T> using min_heap = std::priority_queue<T, std::vector<T>, std::greater<>>;
    min_heap<pair> m_pq;

    sieverator& operator++() {
        cross_off_multiples_of_prime(m_current);
        do {
            ++m_iter;
            m_current = *m_iter;
        } while (is_already_crossed_off(m_current));
        return *this;
    }

    void cross_off_multiples_of_prime(Int value) {
        m_pq.emplace(value * value, value);
    }

    bool is_already_crossed_off(Int value) {
        if (value != m_pq.top().next_crossed_off_value) {
            return false;
        } else {
            do {
                auto x = m_pq.top();
                m_pq.pop();
                m_pq.emplace(x.next_crossed_off_value + x.prime_increment, x.prime_increment);
            } while (value == m_pq.top().next_crossed_off_value);
            return true;
        }
    }

What is a bit concerning is that that function was spending 58% of *its* time in a
libc++ library routine called `__sift_down`! This is because when we `pop` an element
from the priority queue, the Standard specifies that we'll do it by moving the
last element to the top (replacing the top element, which we are popping off),
and then sifting that element down until it reaches its proper place in the new,
one-element-shorter heap. Since it was on the *bottom* of the heap to begin with,
its proper place is probably going to be near the bottom again; so each `pop` costs
us `log N` operations.

And then we immediately emplace a new element, which goes to the end of the heap and
then sifts *up* to *its* proper place! (This is likely to be a no-op: the value we
are emplacing is very large, and its proper place will be near the bottom of the heap.)

So, for each element that we pop-and-then-reemplace in our priority queue, we are
re-shuffling the heap twice. First we sift down an arbitrary element that was already
in its proper place to begin with; and then we add our new element and sift it up
to its proper place. Obviously it would be more efficient if we just replaced the
top element *with* our new element and sifted it down! But this operation is missing
from the standard `priority_queue`, and even missing from the standard `<algorithm>`
header.

In other words, the operation that we are really trying to do is

    namespace nonstd {
        template<class T, class Container, class Compare>
        class priority_queue : public std::priority_queue<T, Container, Compare>
        {
        public:
            template<class... Args>
            void reemplace_top(Args&&... args) {
                this->pop();
                this->emplace(std::forward<Args>(args)...);
            }
        };
    } // namespace nonstd

([Here's the code.](/blog/code/2018-04-27-sieve-reemplace-portable.cpp))
We can rewrite this in terms of the standard heap algorithms, but that doesn't
change anything yet:

    namespace nonstd {
        template<class T, class Container, class Compare>
        class priority_queue : public std::priority_queue<T, Container, Compare>
        {
        public:
            template<class... Args>
            void reemplace_top(Args&&... args) {
                std::pop_heap(c.begin(), c.end(), comp);
                c.back() = T(std::forward<Args>(args)...);
                std::push_heap(c.begin(), c.end(), comp);
            }
        };
    } // namespace nonstd

but that doesn't change anything about the bad behavior yet. We're still
calling `__sift_down` in `pop_heap`, and then calling `__sift_up` in `push_heap`.
Two sifts, where we want just one.

Now, because the Standard doesn't actually specify the exact way in which
the library represents a min-heap in terms of a sequence container, this is where
we have to go slightly non-portable. Let's assume that a min-heap is represented
in the "obvious" way: the root of the heap is at `c.front()` (this much is guaranteed),
and then the root's children are at `c[1]` and `c[2]` (this is technically not
guaranteed but it is "obvious"), and so on. So we can [write our own versions
of `sift_down` and `sift_up`](https://github.com/Quuxplusone/from-scratch/blob/dadc5cc579223ea7fa74f3288f2abde105a6fcd7/include/scratch/bits/algorithm/make-heap.h)
that will interoperate seamlessly with any library's `push_heap` and `pop_heap`,
as long as the library doesn't go out of its way to be malicious.

Once we have these `sift_up` and `sift_down` algorithms, we can avoid the extra sift.
Assuming for convenience that our `sift_down` is spelled *exactly* the same way that
libc++ spells it, we'll write this:

    namespace nonstd {
        template<class T, class Container, class Compare>
        class priority_queue : public std::priority_queue<T, Container, Compare>
        {
        public:
            template<class... Args>
            void reemplace_top(Args&&... args) {
                using std::__sift_down;  // Don't try this at home. Or do. Whatever.
                c.front() = T(std::forward<Args>(args)...);
                __sift_down(c.begin(), c.end(), comp, c.size(), c.begin());
            }
        };
    } // namespace nonstd

([Here's the code.](/blog/code/2018-04-27-sieve-reemplace-nonportable.cpp))
The portable version (with two sifts) finds the 10,000,000th prime in
about 50 seconds. The version above (with one sift) does the same thing
in **28 seconds!**

This huge speedup surprises me, because I argued above that the second sift (the sift-up
of our newly inserted item) should be a no-op. And that's all we're skipping here.
We're still sifting down a very big element from the root down to the bottom half of the
heap; the only difference is that we're not then going and touching *another* element
at the very tail end of the heap. And somehow, omitting that touch saves us about
half of our running time! Either my analysis is wrong, or this is some sort of
[cache effect](https://en.wikipedia.org/wiki/Locality_of_reference). I really cannot
fully explain it. (The Time Profiler profile shows that we are now spending 81% of our
time in `__sift_down`, as opposed to the original 58%. This matches our intuition that
we eliminated a `__sift_up` that was not showing up in the profile, increasing the relative
proportion of time spent inside `__sift_down`. The `__sift_up` wasn't showing up in the profile
because of inlining.)

Anyway, we have now knocked our *original* running time of 97 seconds down to an
improved time of 28 seconds. Here's the itemized bill:

    essential complexity          28 seconds
    premature pessimization tax  +17 seconds    
    priority_queue tax           +22 seconds
    iostreams tax                +30 seconds
    ----------------------------------------
    Total:                        97 seconds

What do you think — should the STL's `priority_queue` be extended to include a
`replace_top` operation?

[Here's a libc++ patch that implements `reemplace_top`](https://github.com/Quuxplusone/libcxx/commit/pq-replace-top),
plus a matching `<algorithm>`-header algorithm which I'm calling `poke_heap` because I can't come up
with any really good name. Feel free to use it, until your friendly neighborhood STL vendor
includes it in their implementation.
