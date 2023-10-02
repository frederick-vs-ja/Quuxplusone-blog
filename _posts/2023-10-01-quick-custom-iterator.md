---
layout: post
title: "A neat technique for custom output iterators"
date: 2023-10-01 00:01:00 +0000
tags:
  pearls
  stl-classic
  today-i-learned
---

Here's a neat tip — credit due to Hank Meng via his blog post
["Custom iterator"](https://biowpn.github.io/bioweapon/2023/06/29/Custom-Iterator.html)
(June 2023). (I've read all four of the posts currently available on
[biowpn.github.io/bioweapon/](https://biowpn.github.io/bioweapon/); they're
all good!)

You know the STL likes to take "source" and "sink" operations — `cin >> x`,
`cout << x`, `v.push_back(x)` — and "lift" them into input and output iterators.
For example, the STL provides [`front_insert_iterator`](https://en.cppreference.com/w/cpp/iterator/front_insert_iterator)
and [`back_insert_iterator`](https://en.cppreference.com/w/cpp/iterator/back_insert_iterator):
when you initialize `it = std::back_inserter(v)`, writing `*it++ = x` will
turn into `v.push_back(x)`. This allows us to compose any STL algorithm
(e.g. `remove_copy_if`) with the notion of pushing onto the front or back of a
container.

    std::list<int> src = {1,2,3,4,5,6,7,8};
    std::list<int> dst;
    std::remove_copy_if(src.begin(), src.end(), std::front_inserter(dst), isOdd);
    assert((dst == std::list<int>{8,6,4,2}));

But suppose we wanted to push into a `priority_queue` instead! `priority_queue` doesn't
have `push_back` or `push_front` methods; it just has `push`. There's no pre-rolled STL
iterator type corresponding to `q.push(x)`. So the traditional approach (in my previous experience) is
to go through all the busywork of implementing a new iterator from scratch.
C++20 makes it easier than C++17, but it's still tedious ([Godbolt](https://godbolt.org/z/YWYcz4jnj)):

    template<class Q>
    struct PushIterator {
      using difference_type = int;
      explicit PushIterator(Q& q) : q_(&q) {}
      PushIterator& operator++() { return *this; }
      PushIterator operator++(int) const { return *this; }
      PushIterator operator*() const { return *this; }
      void operator=(Q::value_type value) const { q_->push(value); }
      Q *q_;
    };

The neat trick I learned from [Hank's blog post](https://biowpn.github.io/bioweapon/2023/06/29/Custom-Iterator.html#output-iterator-using-stdback_insert_iterator)
is that you can leverage `back_insert_iterator` to do the heavy lifting!
Instead of implementing a `PushIterator` that satisfies the many requirements of all
the STL algorithms we might ever want to use (i.e., it must satisfy [`std::output_iterator`](https://en.cppreference.com/w/cpp/iterator/output_iterator)
and meet the [_LegacyOutputIterator_ requirements](https://en.cppreference.com/w/cpp/named_req/OutputIterator) too),
we can simply implement a `PushContainerAdaptor` that satisfies the one simple requirement of
`std::back_insert_iterator` (i.e. it must have a `push_back` method).

> This should be a general rule: Whenever you have the choice of conforming to a lot of
> explicit requirements (e.g. the `output_iterator` requirements) or conforming to a
> very small number of practical requirements (e.g. to have a `push_back` method),
> strongly consider the latter. Prefer "less and simpler" over "more and complex."

Now, the problem not discussed in Hank's blog post is that `back_inserter` doesn't want a
container adaptor; it wants a _native reference_ to a container. So we can't just construct
a temporary adaptor and pass it to `back_inserter`; we need to pass a reference to something
that will be suitably long-lived. A temporary doesn't qualify.

The conforming solution is simply to avoid temporaries and helper functions
([Godbolt](https://godbolt.org/z/rEPv4dsMn)):

    template<class Q>
    struct PushAdaptor {
      using value_type = Q::value_type;
      explicit PushAdaptor(Q& q) : q_(&q) {}
      void push_back(value_type value) const { q_->push(value); }
      Q *q_;
    };

    std::priority_queue<int> dst;
    auto realgood = PushAdaptor(dst);
    std::remove_copy_if(src.begin(), src.end(), std::back_inserter(realgood), isOdd);

But there's also a fun solution! The following is totally UB, but please tell me if you ever see
it fail in practice. [Godbolt](https://godbolt.org/z/os47f8WMr):

    template<class Q>
    struct UBPushAdaptor {
      using value_type = Q::value_type;
      void push_back(value_type value) {
        Q *q = reinterpret_cast<Q*>(this);
        q->push(value);
      }
    };
    template<class Q>
    auto pusher(Q& q) {
      return std::back_inserter(reinterpret_cast<UBPushAdaptor<Q>&>(q));
    }

    std::priority_queue<int> dst;
    std::remove_copy_if(src.begin(), src.end(), pusher(dst), isOdd);
