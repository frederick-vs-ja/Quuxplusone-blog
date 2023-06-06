---
layout: post
title: "Make `operator<=>` ignore a data member"
date: 2023-06-05 00:01:00 +0000
tags:
  c++-style
  operator-spaceship
  paradigm-shift
  pearls
---

Yesterday I was reading [P2685R1 "Scoped Objects."](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2685r1.pdf)
This blog post has nothing at all to do with the point of that paper;
I'd just like to explore one extremely tangential point it raised.
How do we write "basically an aggregate, but with an allocator that is non-salient
for comparison"? P2685R1 §7.1.3 suggests this grotesque snippet:

    friend auto operator<=>(A const & lhs, A const & rhs) noexcept
      -> std::strong_ordering {
        return lhs.data1 < rhs.data1 ? std::strong_ordering::less
             : lhs.data1 > rhs.data1 ? std::strong_ordering::greater
             : lhs.data2 < rhs.data2 ? std::strong_ordering::less
             : lhs.data2 > rhs.data2 ? std::strong_ordering::greater
             : lhs.data3 < rhs.data3 ? std::strong_ordering::less
             : lhs.data3 > rhs.data3 ? std::strong_ordering::greater
             : std::strong_ordering::equal;
        // and ignore the allocator
    }

Of course we should never write an `operator<=>` "case by case" like that.
We should _at least_ know the C++20 idiom, which halves the number of comparisons
involved:

    friend std::strong_ordering operator<=>(const A& lhs, const A& rhs) {
      if (auto r = (lhs.data1 <=> rhs.data1); r != 0) {
        return r;
      } else if (auto r = (lhs.data2 <=> rhs.data2); r != 0) {
        return r;
      } else if (auto r = (lhs.data3 <=> rhs.data3); r != 0) {
        return r;
      } else {
        return (1 <=> 1);
      }
    }

Better still would be to use the "tie" idiom from C++11:

    friend std::strong_ordering operator<=>(const A& lhs, const A& rhs) {
      auto tie = [](auto& x) { return std::tie(x.data1, x.data2, x.data3); };
      return tie(lhs) <=> tie(rhs);
    }
    friend bool operator==(const A& lhs, const A& rhs) {
      auto tie = [](auto& x) { return std::tie(x.data1, x.data2, x.data3); };
      return tie(lhs) == tie(rhs);
    }

But the real "value-semantic purist" way to write this would be to take the
bit we want to behave differently and pull it out into a proper class.
We want the allocator not to contribute to comparison, so we wrap
it up in a class like this ([Godbolt](https://godbolt.org/z/Mjv9T5z5e)):

    struct ComparisonIgnorerBase {
      using CIB = ComparisonIgnorerBase;
      constexpr friend auto operator<=>(const CIB&, const CIB&) = default;
    };

    template<class T>
    struct ComparisonIgnorer : ComparisonIgnorerBase {
      T t_;
      ComparisonIgnorer(T t) : t_(std::move(t)) {}
    };

    struct A {
      using allocator_type = ~~~~;
      int data1_;
      int data2_;
      int data3_;
      ComparisonIgnorer<allocator_type> alloc_;

      friend auto operator<=>(const A&, const A&) = default;
    };

We're extremely comfortable doing this with the lifetime special members —
that is, pulling out lifetime-related responsibilities into proper RAII classes.
This is just an example of doing the same thing with a defaulted comparison operator.
