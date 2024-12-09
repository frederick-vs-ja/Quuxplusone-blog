---
layout: post
title: "When `ranges::for_each` behaves differently from `for`"
date: 2024-12-09 00:01:00 +0000
tags:
  implementation-divergence
  library-design
  ranges
  standard-library-trivia
---

This week I learned an interesting and dismaying fact:
C++11's range-based `for` loop and C++20's `ranges::begin`/`end`
use different protocols to find the "beginning" and "end" of a range!

For the range-based `for` loop (<a href="https://eel.is/c++draft/stmt.ranged">[stmt.ranged]</a>),
the bounds of the loop are determined together, as a pair:

- If `rg` is an array, then we use `rg` and `rg+N` as our bounds.
- Otherwise, if `rg.begin()` and `rg.end()` are both present, then we use `rg.begin()` and `rg.end()`.
- Otherwise, we use `begin(rg)` and `end(rg)`, looked up with ADL-only lookup.

For C++20's `ranges::begin` and `ranges::end`, the bounds are determined
by <a href="https://eel.is/c++draft/range.access.begin">[range.access.begin]</a>
and <a href="https://eel.is/c++draft/range.access.end">[range.access.end]</a>
separately, individually, without any cross-consultation:

- If `rg` is an array, we use `rg` and `rg+N` as our bounds.
- If `rg.begin()` is well-formed and models `input_or_output_iterator`, we use `rg.begin()` as our lower bound.
- Otherwise, if ADL-only `begin(rg)` is well-formed and models `input_or_output_iterator`, we use `begin(rg)` as our lower bound.
- Meanwhile, if `rg.end()` is well-formed and models `sentinel_for<iterator_t<R>>`, we use `rg.end()` as our upper bound.
- Otherwise, if ADL-only `end(rg)` is well-formed and models `sentinel_for<iterator_t<R>>`, we use `end(rg)` as our upper bound.

## "Present" versus "valid"

The first — less important — difference between the two protocols is that
the core-language protocol says "present" where the Ranges library protocol says "well-formed"
(actually, "valid," but I _think_ those are synonyms in this context). So the core-language
protocol is more conservative in cases where `rg.begin()` is present but unusable for some reason;
for example, if it's inaccessible, or deleted, or ambiguous, or returns something that's not an
iterator (like `int`). The C++20 `ranges::begin` will happily ignore that troublesome result and
fall back on `begin(rg)` in that case.

[This Godbolt](https://godbolt.org/z/evWT5vGz6) shows one way this difference could matter:

    class Secret {
      auto begin() { return std::counted_iterator("Core\n", 5); }
      auto end() { return std::default_sentinel; }
      friend void friendly(Secret&);
    };
    auto begin(Secret&) { return std::counted_iterator("Library\n", 8); }
    auto end(Secret&) { return std::default_sentinel; }

Inside the `friendly` function,

    Secret rg;
    for (char c : rg) { putchar(c); }
    std::ranges::for_each(rg, [](char c) { putchar(c); });

have different behaviors: the former accesses the private member functions and prints "Core";
those members are inaccessible from within `for_each`, so the latter falls back on the global
functions and prints "Library".

Outside of `friendly` — say, in `main` — the former finds the member functions inaccessible
and gives a hard compiler error; the latter falls back on the global functions and prints "Library".

## Together versus separate

The more important difference between the two protocols is that
the core-language protocol requires that both bounds be advertised by
the same mechanism, which is a proxy for "implemented by the same person."
If class `C` has an `end` method but no `begin` method, that's probably a
deliberate (strange) choice by the type-author. The core language doesn't allow
you to "override" that choice by providing your own free function `begin(C&)`
at namespace scope, unless you also provide your own free function `end` to match it.

_But the Ranges library does!_ There's no cross-talk between `ranges::begin` and `ranges::end`;
they're determined independently.
(Actually, `ranges::end` does need to recompute `decltype(ranges::begin(rg))`
in order to check `sentinel_for<iterator_t<R>>`; but it doesn't care which _variety of `begin`_
was found by `ranges::begin`, it just cares about the _iterator type_ that resulted.)
This means that `ranges::begin` could find a member function and `ranges::end` a free function,
or vice versa.

This is a more exciting difference, because it means you can write a class where both the
core-language `for` loop and the library facility `ranges::for_each` are well-formed, but
they find different `begin`s and thus do different things.

[This Godbolt](https://godbolt.org/z/jh3YfxMY5) shows one way this difference could matter:

    struct Evil {
      auto begin() { return std::counted_iterator("Library", 7); }
      friend auto begin(Evil&) { return std::counted_iterator("Core", 4); }
      friend auto end(Evil&) { return std::default_sentinel; }
    };

    Evil rg;
    for (char c : rg) { putchar(c); }
    std::ranges::for_each(rg, [](char c) { putchar(c); });

Now, the core-language `for` loop finds a member `rg.begin()` but no `rg.end()`, so it falls
back to the ADL hidden friends `begin(rg)` and `end(rg)`. But `ranges::for_each` determines
`ranges::begin(rg)` and `ranges::end(rg)` independently: `rg.begin()` for the former and
`end(rg)` for the latter. So the core-language loop starts at `begin(rg)` while the library
`for_each` starts at `rg.begin()`.

If you're a working programmer, this is just a weird bit of trivia that should never matter to
you: if it does, you're surely doing something wrong! But if you're a library implementor, this is
a real pain, because it means that there are range types (`std::ranges::range<Evil>` is `true`!)
where you aren't allowed to use the core-language `for` loop to iterate them, because the
core-language `for` loop might iterate over different elements from those that C++20 Ranges sees.
This matters in places like C++23's new `from_range_t` constructors and `.insert_range` methods,
as seen in [the Godbolt above](https://godbolt.org/z/jh3YfxMY5): my understanding is that a
conforming Ranges implementation must print "Library" in all those cases, never print "Core."

This is awful, for both library vendors and users, because instantiating `ranges::for_each` is vastly
slower than just using the core-language control-flow construct. (Merely _including the headers_ to get
a working version of `for_each` is pretty painful!) It would be a better world if library vendors
were somehow permitted to use the core-language `for` loop in their algorithms — although I have
no great ideas how to get there from here.

Hat tips to Tomasz Kamiński and Tim Song for first alerting me to this quirk; and to
Hewill Kang for explaining the `Evil` example [on StackOverflow](https://stackoverflow.com/a/79262269/1424877)
and then immediately filing [libc++ bug #119133](https://github.com/llvm/llvm-project/issues/119133)
to report and discuss all the places libc++ currently prints "Core" when it should print "Library."

---

See also:

* ["Prefer core-language features over library facilities"](/blog/2022/10/16/prefer-core-over-library/) (2022-10-16)
