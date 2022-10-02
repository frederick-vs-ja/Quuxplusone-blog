---
layout: post
title: "Thoughts on `-Wctad-maybe-unsupported`"
date: 2022-10-07 00:01:00 +0000
tags:
  class-template-argument-deduction
  compiler-diagnostics
  llvm
  proposal
  rant
---

As of this writing, there's an ongoing discussion in Clang/libc++ circles
about `-Wctad-maybe-unsupported`. (See [D133425](https://reviews.llvm.org/D133425),
[D133535](https://reviews.llvm.org/D133535).) This is the watered-down version
of `-Wctad` ([D54565](https://reviews.llvm.org/D54565)) that shipped as a compromise back in January 2019
(see [D56731](https://reviews.llvm.org/D56731)) and is intended to help diagnose
unintended uses of C++17 class template argument deduction ([CTAD](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#ctad)).

The problem with CTAD as a feature is that it is easy to use without even realizing
that you're using it — and then, maybe one percent of the time, it does the wrong thing.
My go-to example as of a few months ago is [`reverse_iterator`](/blog/2022/08/02/reverse-iterator-ctad/) (2022-08-02).

    template<class It>
    bool is_palindrome(It first, It last) {
        return std::equal(
            first, last,
            std::reverse_iterator(last),
            std::reverse_iterator(first)
        );
    }

This code is broken: it has undefined behavior ([Godbolt](https://godbolt.org/z/qxajb6MvW))
whenever it is instantiated as e.g.

    std::vector<int> v = {1, 2, 2, 1};
    bool b = is_palindrome(v.rbegin(), v.rend());

Both Clang 9+ and GCC 11+ provide an off-by-default warning named `-Wctad-maybe-unsupported`
intended to catch this kind of bug — or at least, to wave vaguely in the direction of such bugs.
They both use the heuristic that _if a type has no explicit deduction guides, then it might not
intend to support CTAD_, and vice versa _any type with at least one explicit deduction guide
must be intended to support CTAD in all possible cases._ GCC seems to have something else going
on, too, since its `-Wctad-maybe-unsupported` fails to trigger in the `reverse_iterator` case.

The renewed discussion around CTAD diagnostics has caused me to ponder the topic a bit more,
and I think I see a better heuristic for when CTAD is "okay" and when it's not.

----

First, the blaring headline: **I recommend never using CTAD under any circumstances whatsoever!**
Using C++17 with a blanket `-Werror=ctad` option should be just as non-controversial as using
C99 with a blanket `-Werror=vla`. However, as of October 2022, you'll have some difficulty applying
`-Werror=ctad` because neither GCC, nor Clang, nor MSVC, support a `-Wctad` option yet.

CTAD is never _necessary_, but it is easy to apply it by _accident_. The standard library
goes out of its way to provide a `std::make_pair` with the "correct" behavior around `std::ref`;
a `std::make_reverse_iterator` with correct behavior even for iterators that are already reversed;
and view factories like [`std::views::take`](https://en.cppreference.com/w/cpp/ranges/take_view),
`std::views::filter`, and `std::views::all` to deal correctly with
the icky corner cases that couldn't possibly be addressed by raw CTAD on `take_view`,
`filter_view`, and so on.

> Guideline for Ranges: Always, always, always use the short-named factories like `std::views::take`.
> Never attempt to construct a `std::ranges::take_view<V>` by hand.

----

The next level of compromise down from "never" is: Okay, you can use CTAD with a very small
explicit whitelist of types. (Not "templates" — "types"!) Personally I would limit that list
to `std::lock_guard<std::mutex>` and `std::unique_lock<std::mutex>`: it's convenient to be able
to write those as `std::lock_guard` and `std::unique_lock` respectively. However, in a production
codebase I think it would be quite reasonable to simply introduce a typedef, e.g.

    namespace my {
        using lock_guard = std::lock_guard<std::mutex>;
    }

    auto lk = my::lock_guard(m);  // compatible with -Wctad

----

Now I'm going to talk about a level of compromise that is _below what I consider the bar for production code,_
but which I think might actually be palatable to the Clang developers who rejected `-Wctad` in 2019.
This is just a minor tweak on top of the existing `-Wctad-maybe-unsupported` warning.

Observe the difference between these two uses of CTAD:

    std::reverse_iterator rit = v.rbegin(); // A

    auto rit = std::reverse_iterator(it);   // B

I claim that line `A` is "presumably safe," no matter what type `v.rbegin()` returns. Philosophically, it's
just a compile-time assertion about the type of the right-hand expression: it says, "`v.rbegin()`
returns some kind of `reverse_iterator`, and I don't care which; compiler, please figure it out."
It's kind of one step up in explicitness from `auto`, but not going quite all the way. It's semantically
similar to

    std::bidirectional_iterator auto rit = v.rbegin();  // A2

Now, I know it's not _physically interpreted by the compiler_ in the same way! But it's the
same idea in the programmer's mind. The one big difference here is that line `A2` constrains
the actual decayed type of `v.rbegin()` itself, whereas line `A` can involve an implicit conversion.
(Line `A` is one of several places where C++ privileges "converting constructors" over conversion operators;
see [this Godbolt](https://godbolt.org/z/546bfe9z6) for an example.)

Notice that line `A` is safe _even for `reverse_iterator`._ It's not the whole `reverse_iterator` template
that "might not support CTAD"; it's actually perfectly fine to use CTAD on `reverse_iterator` as long
as you're using it only in situations like line `A`, where only implicit conversions are permitted.

Line `B`, on the other hand, is essentially unsafe, because it does an _explicit functional-style cast_
from the type of `it` to "I don't care; compiler please figure it out." Explicit casts are not
guaranteed to be safe in the same way as implicit conversions! For an extreme example, see
["Hidden `reinterpret_cast`s"](/blog/2020/01/22/expression-list-in-functional-cast/) (2020-01-22).
The effect in this case is that the programmer loses control over whether CTAD is going
to choose the `explicit` converting-constructor candidate or the implicit copy deduction candidate.
The outcome will depend on the concrete type of `it`.

Notice that the worst outcome (UB) happens precisely when the compiler _does_ prefer the
implicit copy deduction candidate on line `B`. So the diagnosable sin here isn't that the compiler
_chooses_ an `explicit` constructor as the best match; it's that the programmer uses the _syntax_ of an explicit cast.

So, GCC and Clang should consider tweaking their `-Wctad-maybe-unsupported` diagnostics to permit
line `A` and to diagnose line `B`:

* If CTAD is used in a situation that permits only implicit conversions (such as copy-initialization),
    then it's always permitted and never diagnosed by `-Wctad-maybe-unsupported`.

* If CTAD is used in a situation that permits explicit conversions (such as direct-initialization or
    a functional-style cast expression, with or without curly braces), then it's potentially dangerous,
    and may be diagnosed by `-Wctad-maybe-unsupported`. We still need a heuristic to distinguish
    e.g. `reverse_iterator(it)` from `move_iterator(it)`, and today's heuristic of "does it have
    any deduction guides?" is probably still a fine one.

As presented here, this is a strict _relaxation_ of these compilers' current rules (as I understand them);
that is, it permits CTAD in more (safe) cases while not forbidding it in any new cases. However,
as I said above, I don't understand why GCC doesn't warn about `reverse_iterator(it)` today.

----

It might also be a good idea to give the library author some way to proactively disable CTAD
for a class template — to say "This class has a few explicit deduction guides for the safe cases,
but I'd like a diagnostic if CTAD ever selects an implicitly generated deduction guide."

Vice versa, it might be useful to give the author of an aggregate (which permit CTAD since C++20)
a cleaner way to opt _in_ to CTAD:

    template<class T>
    struct Point { T x; T y; };

    template<class T> Point(T, T) -> Point<T>;
      // Without the line above, we get
      // -Wctad-maybe-unsupported below

    Point p = {1, 2};

What if the clumsy and error-prone explicit deduction guide could be replaced with a simple
vendor-specific attribute `[[clang::ctad_supported]]`?

> For non-aggregates we can get away with an inconspicuous `Point() -> Point<void>`,
> but [P2082 "Fixing CTAD for aggregates"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2082r1.pdf)
> made it so that adding any deduction guide to a class disables the aggregate deduction candidate!
> Therefore, for aggregates, we need to explicitly and error-prone-ly reproduce the aggregate
> deduction candidate's signature in order to satisfy `-Wctad-maybe-unsupported`.

----

In conclusion, though, let me repeat my top-line advice:
*I recommend never using CTAD under any circumstances whatsoever!*
If you have the ability to use `-Wctad` or some similar option, to forbid all uses of CTAD
throughout your production codebase, I strongly recommend that you do so.
