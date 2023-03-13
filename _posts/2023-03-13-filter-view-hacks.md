---
layout: post
title: "Iterating and inverting a const `views::filter`"
date: 2023-03-13 00:01:00 +0000
tags:
  cpplang-slack
  pearls
  ranges
excerpt: |
  Via Alexej Harm on Slack, two little hacks for dealing with `std::ranges::filter_view`.

  First, recall that `filter_view` isn't const-iterable — because its iterators keep pointers into
  the view itself, and modify it as you iterate. So if you are
  ["constexpr'ing all the things"](https://www.youtube.com/watch?v=PJwd4JLYJJY)
  (not that I recommend that), how do you iterate a `const filter_view`?
---

Via Alexej Harm on Slack, two little hacks for dealing with `std::ranges::filter_view`.

First, recall that `filter_view` isn't const-iterable — because its iterators keep pointers into
the view itself, and modify it as you iterate. So if you are
["constexpr'ing all the things"](https://www.youtube.com/watch?v=PJwd4JLYJJY)
(not that I recommend that), how do you iterate a `const filter_view`?

    constexpr auto isPrint = [](auto c) { return std::isprint(c); };
    constexpr auto isXDigit = [](auto c) { return std::isxdigit(c); };
    constexpr auto hexdigital = std::views::filter(isXDigit);
    constexpr auto printable = std::views::filter(isPrint);

    constexpr auto digits = std::views::iota('\0')
                          | std::views::take(256)
                          | hexdigital;

    for (char c : digits) ~~~             // Error
    for (char c : digits | printable) ~~~ // Error

C++23 to the rescue!

    for (char c : auto(digits)) ~~~             // OK
    for (char c : auto(digits) | printable) ~~~ // OK

Here we aren't trying to iterate over the const-qualified `digits`: we're asking the compiler
to make us a copy (just like `int(i)` makes a prvalue copy of `i`), and then iterating over
that copy.

----

Second, given an existing `filter_view`, can we "invert its sense"? For example, given `digits`
as above, can we easily iterate over the non-digits? It turns out that C++20 `filter_view`
exposes enough public getters to make this [possible](https://godbolt.org/z/bsdrvzhYs)!

    constexpr auto rest = [](auto fv) {
        return fv.base() | std::views::filter(std::not_fn(fv.pred()));
    };

    for (char c : rest(digits)) ~~~             // OK
    for (char c : rest(digits) | printable) ~~~ // OK

Notice that we're using `fv.base()` on the `filter_view` itself, to extract the base view.
We might have written `ranges::subrange(fv.begin().base(), fv.end().base())` instead,
but that would cause dangling-pointer [bugs](https://godbolt.org/z/PMhhfj83c)
if those iterators referred back into something captured inside `fv`. We should never do that.

> Never prematurely "debone" a range type by splitting it into `begin` and `end`.
> Do that only when you're about to traverse it, if ever at all.

For greater applicability to move-only views such as [`owning_view`](https://en.cppreference.com/w/cpp/ranges/owning_view),
we might consider calling `std::move(fv).base()` instead of `fv.base()`,
or taking `auto&& fv` and calling `decltype(fv)(fv).base()`.
As always, there's a tradeoff between generality and maintainability.
