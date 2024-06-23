---
layout: post
title: 'Tag dispatch versus concept overloading'
date: 2021-06-07 00:01:00 +0000
tags:
  concepts
  metaprogramming
  templates
excerpt: |
  In Bjarne Stroustrup's paper
  [P0557R1 "Concepts: The Future of Generic Programming"](https://www.stroustrup.com/good_concepts.pdf) (January 2017),
  section 6 is titled "Concept Overloading," which Stroustrup distinguishes from the
  traditional approach "using helper functions and tag dispatch."
  More subtly, Stroustrup seems to distinguish between true "overloading" (asking the compiler
  to resolve among multiple viable overloads) and other means of giving-two-things-the-same-name:

  > Where we cannot overload, we need to use
  > workarounds (e.g., traits, `enable_if`, or helper functions).

  I'll present two problems, each with two solutions (one in C++11 and one in C++20),
  which I think illustrate something interesting about "concept overloading" versus...
  other things.
---

In Bjarne Stroustrup's paper
[P0557R1 "Concepts: The Future of Generic Programming"](https://www.stroustrup.com/good_concepts.pdf) (January 2017),
section 6 is titled "Concept Overloading," which Stroustrup distinguishes from the
traditional approach "using helper functions and tag dispatch."
More subtly, Stroustrup seems to distinguish between true "overloading" (asking the compiler
to resolve among multiple viable overloads) and other means of giving-two-things-the-same-name:

> Where we cannot overload, we need to use
> workarounds (e.g., traits, `enable_if`, or helper functions).

I'll present two problems, each with two solutions (one in C++11 and one in C++20),
which I think illustrate something interesting about "concept overloading" versus...
other things.


## 1. Absolute value for signed and unsigned

Let's write a function template that returns the absolute value
of its integral input. If the type of the input is signed, we
compute `(x < 0) ? -x : x`. If it's unsigned, we simply return `x`.


### Tag-dispatch solution

    // A
    template<class T>
    T abs_impl(T x, std::false_type) {
        return (x < 0) ? -x : x;
    }

    template<class T>
    T abs_impl(T x, std::true_type) {
        return x;
    }

    template<class T>
    T abs_value(T x) {
        return abs_impl(x, std::is_unsigned<T>{});
    }

    int main() {
        assert(abs_value(-42) == 42);
        assert(abs_value(42u) == 42u);
    }


### C++20 Concepts solution

    // B
    template<std::signed_integral T>
    T abs_value(T x) {
        return (x < 0) ? -x : x;
    }

    template<std::unsigned_integral T>
    T abs_value(T x) {
        return x;
    }

    int main() {
        assert(abs_value(-42) == 42);
        assert(abs_value(42u) == 42u);
    }

Notice that no type can satisfy both
[`signed_integral`](https://en.cppreference.com/w/cpp/concepts/signed_integral) and
[`unsigned_integral`](https://en.cppreference.com/w/cpp/concepts/unsigned_integral)
at the same time, so this Concepts-based solution is equivalent to the C++14 `enable_if`
approach denigrated by Stroustrup as a "workaround":

    // C
    template<class T, std::enable_if_t<std::is_signed<T>::value, int> = 0>
    T abs_value(T x) {
        return (x < 0) ? -x : x;
    }

    template<class T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0>
    T abs_value(T x) {
        return x;
    }

    int main() {
        assert(abs_value(-42) == 42);
        assert(abs_value(42u) == 42u);
    }

In this kind of overload set, all the constraints are mutually exclusive,
such that no more than one viable candidate is ever present. Overload resolution
never has to choose the "best" overload from among a candidate set.
As I understand it, this is not what we mean by "concept overloading."


## 2. Advance for forward and random-access iterators

Let's write a function template that returns its iterator argument
advanced by two positions.
For the general case of a forward iterator, we compute `++it` twice.
But if the input is additionally a random-access iterator, we can simply return `it + 2`.


### Tag-dispatch solution

    // D
    template<class It>
    It plus2_impl(It it, std::forward_iterator_tag) {
        ++it; ++it;
        return it;
    }

    template<class It>
    It plus2_impl(It it, std::random_access_iterator_tag) {
        return it + 2;
    }

    template<class It>
    It plus2(It it) {
        using Tag = typename std::iterator_traits<It>::iterator_category;
        return plus2_impl(it, Tag{});
    }

The term "tag dispatch" is used to refer both to snippet `D`
and to snippet `A` above. But notice the difference (not reflected
in our terminology): In `A`, our `abs_impl` helpers were designed so
that only one of them would be viable. In `D`, our `plus2_impl` helpers
are designed so that the first overload is always viable, and then
the second overload is sometimes _also_ viable; when it is, it's the
better match ([over.ics.rank/4.4.4](https://timsong-cpp.github.io/cppwp/n4868/over.match.best#over.ics.rank-4.4.4)).


### C++20 Concepts solution (concept overloading)

    // E
    template<std::forward_iterator It>
    It plus2(It it) {
        ++it; ++it;
        return it;
    }

    template<std::random_access_iterator It>
    It plus2(It it) {
        return it + 2;
    }

In this overload set, the constraints are not mutually exclusive.
Like `D`, snippet `E` relies on overload resolution to choose the
"best" overload from among a candidate set. Rather than rely on
the best-match rules regarding inheritance, `E` relies on the fact
that the second overload's constraint _subsumes_ the first overload's
constraint, and thus
([over.match.best.general/2.6](https://timsong-cpp.github.io/cppwp/n4868/over.match.best#general-2.6))
the second overload is the best match whenever it is viable at all.

My understanding is that _this_ is what Stroustrup refers to as
"concept overloading."


## Final thoughts

> UPDATE, 2021-06-08: I've changed my snippets `F`, `G`, and `H` to
> relax the `signed_integral` constraint into `integral`; originally, I'd relaxed
> the `unsigned_integral` constraint. Reddit commenter "Quincunx271" rightly
> [pointed out](https://www.reddit.com/r/cpp/comments/numb1c/tag_dispatch_versus_concept_overloading/h0ynpyj)
> that that way was strictly worse. Our new version of `F` has one generic overload
> that works correctly for all integral types, plus a faster overload for unsigned types.
> (This is the same overall pattern as `D` and `E`.)
> My original post's `F` had a generic overload that worked correctly for unsigned
> types but _incorrectly_ for signed types, and then an overload "rescuing" the correct
> behavior for signed types. That was unnecessarily silly of me.
>
> Reddit commenter "sphere991" observes that in real life we could use `if constexpr`.
> I absolutely agree; I prefer `if constexpr` over any multi-function
> approach. But this post wasn't about `if constexpr`; it was about "concept overloading." :)

It seems to be the current wisdom that if you're writing an
overload set with C++20 Concepts, you needn't go out of your way
to make your constraints mutually exclusive. I claim that all of my
examples are reasonable C++20; but I do have the impression that some
commentators would go so far as to remove the constraint from one
or the other of the overloads in snippet `B`, or relax it until
it was no longer mutually exclusive. Thus:

    // F
    template<std::integral T>  // note: NOT signed_integral
    T abs_value(T x) {
        return (x < 0) ? -x : x;
    }

    template<std::unsigned_integral T>
    T abs_value(T x) {
        return x;
    }

The tag-dispatch equivalent would be to replace `false_type` with an ellipsis:

    // G
    template<class T>
    T abs_impl(T x, ...) {
        return (x < 0) ? -x : x;
    }

    template<class T>
    T abs_impl(T x, std::true_type) {
        return x;
    }

    template<class T>
    T abs_value(T x) {
        return abs_impl(x, std::is_unsigned<T>{});
    }

or use a template of two parameters:

    // H
    template<class T, class U>
    T abs_impl(T x, U) {
        return (x < 0) ? -x : x;
    }

Personally, I would vastly prefer to read snippet `A` than
to read either `G` or `H`.

> I have used `H` in situations dispatching on multiple bools
> at a time — where we want to do one thing if `b1` and `b2`
> are both true, a different thing if `b1` is true and `b2` is false,
> and a third thing if `b1` is false regardless of `b2`.)

----

For me _personally, so far,_ my attitude carries over into C++20 Concepts:
I find snippet `B` strictly more understandable at a glance than
snippet `F`, because you can understand each candidate's purpose
in isolation, without even knowing that the other candidate exists.
However, in cases where the overloads express a _natural_ hierarchy
(by which I mean "a hierarchy that the educated reader ought to know
to be on the lookout for"), concept overloading could be the simplest
approach. For example, I find snippet `E` reasonably understandable.
I would not be tempted to rewrite it merely to introduce
mutually exclusive constraints:

    // J
    template<std::forward_iterator It>
        requires (!std::random_access_iterator<It>)
    It plus2(It it) {
        ++it; ++it;
        return it;
    }

    template<std::forward_iterator It>
        requires std::random_access_iterator<It>
    It plus2(It it) {
        return it + 2;
    }

Although honestly, if I saw that a coworker had already written snippet `J`,
I would not be tempted to change it back to snippet `E`!

Concept overloading allows you to increase the number of viable candidates
in your overload sets, without introducing artificial inheritance hierarchies
of tag types à la `std::forward_iterator_tag` or
[`priority_tag`](https://codereview.stackexchange.com/a/173421/16369).
But ultimately it is just another tool in the C++ toolbox; you can be
_allowed_ to use it, without feeling _obliged_ to use it.
