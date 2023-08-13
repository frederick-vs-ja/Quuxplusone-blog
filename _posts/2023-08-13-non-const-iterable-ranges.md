---
layout: post
title: "Some C++20 ranges aren't const-iterable"
date: 2023-08-13 00:01:00 +0000
tags:
  library-design
  pitfalls
  ranges
excerpt: |
  Via [StackOverflow](https://stackoverflow.com/questions/76882853/why-cant-rangesbegin-be-called-on-a-const-any-view/):
  Newcomers to C++20 Ranges are often surprised that some ranges cannot be "const-iterated."
  But it's true!

      template<std::ranges::range R>
      void foreach(const R& rg) { // Wrong!
        for (auto&& elt : rg) { }
      }
---

Via [StackOverflow](https://stackoverflow.com/questions/76882853/why-cant-rangesbegin-be-called-on-a-const-any-view/):
Newcomers to C++20 Ranges are often surprised that some ranges cannot be "const-iterated."
But it's true!

    template<std::ranges::range R>
    void foreach(const R& rg) { // Wrong!
      for (auto&& elt : rg) { }
    }

C++98's tricky iterators liked to store "state" inside the iterator object; e.g. `istream_iterator<T>` stores
a whole `T` inside the iterator, and copies the `T` when you copy the iterator. Instead, C++20 Ranges likes
to store state inside the range object; e.g. [`ranges::istream_view<T>`](https://en.cppreference.com/w/cpp/ranges/basic_istream_view)
stores that `T` inside the `istream_view`,
and the `istream_view`'s iterators merely hold a reference to it. This means the iterators of an `istream_view<T>`
are safely-and-cheaply movable even when `T` is not. ([Godbolt](https://godbolt.org/z/KajcssEhY).) Thus, incrementing
an `istream_view::iterator` modifies the `istream_view` itself. So the mere act of handing out an iterator grants
permission to modify the view, and thus can't be done on a `const` view object. `istream_view::begin()` is a
non-const member function.

----

Another classic example of storing "state" inside a view is [`filter_view`](https://en.cppreference.com/w/cpp/ranges/filter_view).
When you're filtering an input range, `filter_view` simply stores a view of the underlying range and the
predicate on which to filter. Its `begin()` returns a `filter_view::iterator` to the first element of the underlying range
that satisfies the predicate. The iterator skips over elements that don't match the predicate, so its `operator++` can take
up to linear time... and so can its `begin()`. But you only ever call `begin()` once, so that's not a problem.

When you're filtering a forward range, it gets tricky! A forward range is multi-pass, so it's theoretically possible the
user might call `begin()` a bunch of times in a row. For efficiency, therefore, `filter_view` [is required to cache](https://eel.is/c++draft/range.filter#view-5)
the computed value of `begin()` to make each subsequent call O(1). (In general you still _won't_ call `begin()` more than once;
but if you ever did, you'd see it be faster the second time.)

We can observe the cost of that cache by asking for the `sizeof` a `filter_view` over an iterator that claims to be single-pass,
versus the same view over an iterator that claims to be multi-pass. [Godbolt](https://godbolt.org/z/E8YqYYn4P):

    template<bool B>
    struct DevZeroIterator {
      using value_type = int;
      using iterator_category = std::conditional_t<B,
        std::forward_iterator_tag,
        std::input_iterator_tag>;
      using difference_type = int;
      int operator*() const { return 0; }
      auto& operator++() { return *this; }
      auto operator++(int) { return *this; }
      bool operator==(const DevZeroIterator&) const = default;
    };

    auto v1 = std::views::counted(DevZeroIterator<true>(), 10);
    auto v2 = std::views::counted(DevZeroIterator<false>(), 10);
    auto f1 = v1 | std::views::filter(std::identity());
    auto f2 = v2 | std::views::filter(std::identity());
    printf("%zu, %zu\n", sizeof(f1), sizeof(f2));
      // Microsoft: 28, 16
      // libstdc++: 20, 8
      // libc++:    12, 4

At first glance, this suggests to me that there might be a market for some kind of `views::singlepass(r)` that downgrades
a range's iterator type all the way to `input_iterator_tag`, in order to avoid paying for caching at various levels.
But at second glance, it seems wrong to strip the _traversal_ operations like `operator+=` from an iterator simply because
we intend to iterate it in only a _single pass_. During the C++11 cycle, [Boost.Iterator](https://www.boost.org/doc/libs/1_83_0/libs/iterator/doc/new-iter-concepts.html)
had some thoughts about how to separate the orthogonal aspects that the C++98/20 iterator model mashes together;
but I don't think anything ever came of that idea, even within Boost.

Anyway, in the case of `filter_view`, incrementing the iterator doesn't modify the view object. It's the mere act of
computing `begin()`'s return value that modifies the view, by updating the cache.

> Yes, we could make the cache `mutable`, but then we'd have to guard it with
> [`once_flag`](/blog/2020/10/23/once-flag/) to prevent data races. You'd have to
> link against pthreads just to `views::filter` an array. That'd be ridiculous.

----

A third example of a non-const-iterable range is `ranges::subrange`. The job of `subrange` is to
temporarily hold onto an iterator/sentinel pair and hand them back out when you call `begin()` resp. `end()`.
If the iterator type is move-only (like `istream_view::iterator` above), then `subrange::begin()` will
move it out, which modifies the `subrange` object, so again `subrange::begin()` must be non-const.

> Yes, this happens ([Godbolt](https://godbolt.org/z/G8498ofEG)) even when the `subrange` is an lvalue.
> In normal C++, lvalue-ness means you might look at that value later. But in Ranges, value category
> is [a proxy for lifetime](/blog/2019/03/11/value-category-is-not-lifetime/) — that's why we always
> take ranges by forwarding reference but treat them as lvalues inside the function body — and "look-at-later-ness"
> is communicated strictly by the multi-pass-ness (forward-ness) of the iterator type. `subrange::begin()`
> is allowed to modify the subrange in this case because we know it's an input range — it would be a
> semantic error to call `begin()` a second time.

    template<bool B>
    struct DevZeroIterator {
      using value_type = int;
      using difference_type = int;
      explicit DevZeroIterator() = default;
      DevZeroIterator(const DevZeroIterator&) requires B = default;
      DevZeroIterator(DevZeroIterator&&) = default;
      DevZeroIterator& operator=(const DevZeroIterator&) = default;
      int operator*() const { return 0; }
      auto& operator++() { return *this; }
      void operator++(int) {}
      bool operator==(std::default_sentinel_t) const { return false; }
    };

    auto r1 = std::ranges::subrange(DevZeroIterator<true>(), std::default_sentinel);
    auto r2 = std::ranges::subrange(DevZeroIterator<false>(), std::default_sentinel);
    static_assert(std::ranges::range<const decltype(r1)>);
    static_assert(!std::ranges::range<const decltype(r2)>);

----

Bottom line: Never take arbitrary ranges (not even views, which might be `filter_view`;
not even borrowed ranges, which might be `subrange`) by `const&`. C++20 Ranges are
designed always to be taken by forwarding reference, and then iterated as (possibly-modifiable) lvalues.

    template<std::ranges::range R>
    void foreach(const R& rg) { // Wrong!
      for (auto&& elt : rg) { }
    }

    template<std::ranges::range R>
    void foreach(R&& rg) {      // Right!
      for (auto&& elt : rg) { }
    }

Of course this doesn't apply if you know what kind of object you'll receive.
If you _know_ you're taking a `std::vector<int>`, take by `const&`. If you _know_ you're
taking a `string_view`, [take by value](/blog/2021/11/09/pass-string-view-by-value/).
The advice to take ranges by forwarding reference applies only to STL-algorithm-like templates
dealing with ranges of completely unknown type `R`.

----

See also:

* ["Value category is not lifetime"](/blog/2019/03/11/value-category-is-not-lifetime/) (2019-03-11)
* ["`for (auto&& elt : range)` Always Works"](/blog/2018/12/15/autorefref-always-works/) (2018-12-15)
* ["Iterating and inverting a const `views::filter`"](/blog/2023/03/13/filter-view-hacks/) (2023-03-13)
