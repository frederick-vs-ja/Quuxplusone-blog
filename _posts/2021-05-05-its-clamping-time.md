---
layout: post
title: "Semantically ordered arguments should be lexically ordered too"
date: 2021-05-05 00:01:00 +0000
tags:
  cppnow
  library-design
  rant
  standard-library-trivia
---

In his C++Now talk
["Preconditions, Postconditions, Invariants: How They Help Write Robust Programs,"](https://cppnow2021.sched.com/event/hhkp/)
Andrzej Krzemieński used as an example a function `inRange` with signature

    bool inRange(int val, int lo, int hi)
        [[pre: lo <= hi]]
    {
        return (lo <= val && val <= hi);
    }

Part of why we want to specify a precondition here is that it is astoundingly
easy for the human programmer to mix up the order of their arguments:

    void assertInConfiguredRange(int existingValue) {
        int lower = config.get("LOWER");
        int upper = config.get("UPPER");
        assert(inRange(lower, upper, existingValue)); // oops
    }

Besides adding a runtime-checkable precondition, Andrzej also suggested the
present-day solution of introducing a strong type:

    struct IntRange { int lo; int hi; };
    bool inRange(int val, IntRange r);

    assert(inRange({lower, upper}, existingValue)); // compile-time error

    assert(inRange(existingValue, {lower, upper})); // OK

But if you're stuck with three integers (maybe for performance,
or C compatibility, or something), then I claim (completely tangential to
the point of Andrzej's talk) that the original `inRange` puts those three
integer arguments in the _wrong order!_


## Lexical order should match expected semantic order

The reason we write

    return lo <= val && val <= hi;

instead of, say,

    return val <= hi && val >= lo;

is that the former puts the numbers in "number-line order": we expect
that `val`'s _value_ is (semantically, numerically) between `lo`'s and `hi`'s, so we
place `val`'s _identifier_ (physically, lexically) between `lo`'s and `hi`'s.
Languages such as Python and Raku (but maybe [only](https://softwareengineering.stackexchange.com/questions/316969/)
those two?) explicitly support this idiom via "chained comparisons":

    return lo <= val <= hi

The same idiom can be used for function parameter ordering.
In fact we see this idiom consistently throughout the entire Stepanov-era STL:

    std::rotate(first, new_first, last)
    std::partial_sort(first, output_last, input_last)
    std::nth_element(first, pos, last)
    std::inplace_merge(first1, last1_also_first2, last2)

There's no _physical_ reason Stepanov couldn't have written `rotate(first, last, new_first)` —
the computer wouldn't care! But _humans_ find it easier to reason about things when their
physical lexical order matches their expected semantic order.

We also see this idiom in [Howard Hinnant's `combinations.h` library](https://howardhinnant.github.io/combinations/combinations.html):

    for_each_permutation(first, mid, last, callback)

We also see it in [Walter E. Brown's preferred `min` and `max` semantics](https://www.youtube.com/watch?v=e-TNCbX8mOQ&t=504s).
Notice that `min` below is `std::min`, but `max` below is _not_ the standard `std::max`:

    template<class T> auto& min(const T& a, const T& b) { return b < a ? b : a; }
    template<class T> auto& max(const T& a, const T& b) { return b < a ? a : b; }


## Except for `std::clamp`

C++17 `std::clamp` muffed it.

    std::clamp(value, lo, hi)

This is exactly the antipattern that led to Andrzej's original example.
The API is [easy to misuse](https://www.oreilly.com/library/view/97-things-every/9780596809515/ch55.html),
and so the implementor has to resort to safety nets like preconditions and
assertions. (Not that you should _avoid_ writing safety nets, even in
hard-to-misuse code! But writing hard-to-misuse APIs turns our safety
nets from "must-haves" into "nice-to-haves.")

René Rivera pointed out to me that [OpenGL's `clamp` function](https://docs.gl/sl4/clamp)
makes the same API mistake, and so C++'s `std::clamp` was probably designed to
copy that familiar (if unfortunate) API.

I noticed something interesting in the OpenGL API docs:

> The returned value is computed as `min(max(x, minVal), maxVal)`.

Now, _my_ preferred mnemonic for clamp-style functions is

    int myClamp(int lo, int mid, int hi) {
        return min(max(lo, mid), hi);
    }

That is, you write `min` and `max` in order; and then you just start listing
out the arguments in order: `lo` then `mid` (then we run out of parameter
slots for `min` and have to close the parentheses) then `hi` (and close those
parentheses).

It was remarkable to me that OpenGL uses the same mnemonic! Since `max` is
commutative, it doesn't matter if you take your arguments in the order `lo, mid, hi`
or `mid, lo, hi`.

    int myClamp(int lo, int mid, int hi) {
        return min(max(lo, mid), hi);
    }

    int glClamp(int mid, int lo, int hi) {
        return min(max(mid, lo), hi);
    }

But if you wanted to take "range first, value second," then you'd have to find
a different mnemonic.

    int brokenClamp(int lo, int hi, int mid) {
        return min(max(lo, hi), mid); // oops!
    }


## Combinatorics

For ease of reading, let's rename our parameters from `lo, mid, hi` to `a, b, c`.
We have six possible parameter orderings, each with a more or less "mnemonic" implementation
that uses the arguments in that specific order:

    return min(max(a, b), c);  // my preference
    return max(a, min(c, b));
    return min(max(b, a), c);  // OpenGL, C++17
    return max(min(b, c), a);
    return min(c, max(a, b));
    return max(min(c, b), a);

When `b` appears lexically in the middle — as it should! — then we have two
implementation options; but I hope you'll agree that the left-hand option below
is "more mnemonic" than the right-hand option.

    return min(max(a, b), c); <===> return max(a, min(b, c));
    return max(min(c, b), a); <===> return min(c, max(b, a));

What's the point of all this combinatorial stuff? None, really.

But what are the takeaways overall?

> Do as `std::rotate` does.

> The lexical ordering of a parameter list should match the expected
> or logical ordering of the parameters' values.

    int oneTrueClamp(int lo, int mid, int hi) {
        return min(max(lo, mid), hi);
    }

    bool oneTrueInRange(int lo, int mid, int hi) {
        return lo <= mid && mid <= hi;
    }

---

See also:

* ["Semantically ordered arguments, round 2"](/blog/2025/01/05/clamp-round-2/) (2025-01-05)
