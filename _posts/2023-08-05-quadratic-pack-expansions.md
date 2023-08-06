---
layout: post
title: 'Fun with quadratic pack-expansions'
date: 2023-08-05 00:01:00 +0000
tags:
  compile-time-performance
  cpplang-slack
  metaprogramming
  variadic-templates
---

From [the cpplang Slack](https://cppalliance.org/slack/) tonight, some fun with variadic
consteval utility functions.

> By the way, I think I am now suitably convinced that as C++98 `const` variables are
> improved by C++11 `constexpr`, so C++14 `constexpr` functions are improved by C++20 `consteval`.
> If you're writing a function that you know will only ever be used at compile time, and
> [you aren't gonna need it](https://en.wikipedia.org/wiki/You_aren%27t_gonna_need_it)
> to be evaluable at runtime, then you should mark it `consteval`, not `constexpr`, because
> `consteval` is more specific to your intent. You'll get a noisy error when someone tries
> to use your function in a way you didn't intend (i.e., at runtime), and then you can
> consciously decide whether you want to support that use-case or not.

Using C++17 [fold-expressions](https://en.cppreference.com/w/cpp/language/fold),
we can write linear utility functions like:

    consteval bool all_of(const auto& f, const auto&... xs) {
      return (f(xs) && ...);
    }

    consteval bool contains(const auto& n, const auto&... hs) {
      return ((hs == n) || ...);
    }

    consteval int count(const auto& n, const auto&... hs) {
      return (0 + ... + (hs == n));
    }

    static_assert(count(1, 3,1,4,1,6) == 2);
    static_assert(count(2, 3,1,4,1,6) == 0);

> Note that `n` stands for "needle" and `hs` stands for "haystack"
> (pluralized with `-s` because it is a pack of multiple elements).

It gets trickier when we want to ask whether a pack contains _any_ duplicates
(that is, any elements whose `count` is greater than one). Here's a "recursive template"
way to do it:

    consteval bool has_any_duplicates() { return false; }
    consteval bool has_any_duplicates(const auto& n, const auto&... hs) {
      return ((n == hs) || ...) || has_any_duplicates(hs...);
    }

    static_assert(has_any_duplicate(3,1,4,1,6));
    static_assert(!has_any_duplicate(2,7,1,8,3));
    static_assert(has_any_duplicate(9,9,9));
    static_assert(!has_any_duplicate(9));
    static_assert(!has_any_duplicate());

But see ["Iteration is better than recursion"](/blog/2018/07/23/metafilter/) (2018-07-23).
A not-so-recursive-looking approach:

    constexpr auto has_duplicate_of(const auto& value) {
      return [&](const auto&... hs) {
        return count(value, hs...) >= 2;
      };
    }
    consteval bool has_any_duplicates(const auto&... hs) {
      return (has_duplicate_of(hs)(hs...) || ...);
    }

> As of this writing, Clang accepts the code above as written. GCC and MSVC both require
> you to explicitly `consteval`-qualify the lambda: `[&](const auto&... hs) consteval {`.
> I believe this is merely because they haven't yet implemented
> [P2564 "consteval needs to propagate up"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2564r3.html) (Revzin, 2022).

In fact we can get rid of the `has_duplicate_of` helper.

    consteval bool has_any_duplicates(const auto&... hs) {
      return ((count(hs, hs...) >= 2) || ...);
    }

Can we get rid of the `count` helper? Well, we can certainly inline it as a lambda:

    consteval bool has_any_duplicates(const auto&... hs) {
      return ([&](const auto& n) { return (0 + ... + (hs == n)) >= 2; }(hs) || ...);
    }

In fact we can even move the introduction of `n` from a function argument into a capture,
producing this code-golfed monstrosity ([Godbolt](https://godbolt.org/z/Wj66abKvc)):

    consteval bool has_any_duplicates(const auto&... hs) {
      return ([&,&n=hs]{ return (0 + ... + (hs == n)) >= 2; }() || ...);
    }

But I think we cannot eliminate the lambda entirely. I think we always need
one layer of curly braces somewhere to introduce a new name for the thing I'm calling `n` —
the "loop variable of the outer loop" — so that we can distinguish it from the `hs` being
expanded in the "inner loop." Unless I'm mistaken, we can't use the name `hs` to refer
to _both_ the "outer" and "inner" loop variables without ambiguity; and we can't introduce
the new name `n` without a curly-braced scope (either a lambda or a named helper function
like `count`). But if I'm just failing to see a trick, please let me know!

----

The sharp-eyed reader will exclaim, "That post really didn't have anything to do with
`consteval`! You could have removed all the `consteval` keywords and replaced `static_assert`
with `assert`; all the pack-expansion stuff would apply just as well without dragging in
`consteval`." True; but restricting these functions to compile time allowed us to ignore
their awful runtime performance, regarding both wasted CPU cycles and template bloat.
Benchmarking the functions' compile-time performance is left as an exercise for the reader.
