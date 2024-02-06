---
layout: post
title: 'A note on feature-test macros'
date: 2024-02-06 00:01:00 +0000
tags:
  feature-test-macros
  wg21
excerpt: |
  Previously on this blog: ["C++2a idioms for library feature detection"](/blog/2018/10/26/cpp-feature-macros/) (2018-10-26).

  Every C++ implementation is required to provide feature-test macros indicating which
  features it supports. These come in two flavors:

  - Core-language macros such as `__cpp_generic_lambdas`,
      specified in [[cpp.predefined]](https://eel.is/c++draft/cpp.predefined#tab:cpp.predefined.ft).
      These are provided by the compiler vendor via the same mechanism as `__FUNCTION__` and `__GNUC__`.

  - Library macros such as `__cpp_lib_ranges_iota`,
      specified in [[version.syn]](https://eel.is/c++draft/version.syn).
      These are provided by the library vendor via the same mechanism as `assert` and `_GLIBCXX_RELEASE`.
      They all begin with `__cpp_lib_`. The easiest way to get all of them at once is
      to `#include <version>` (since C++20).
---

{% raw %}
Previously on this blog: ["C++2a idioms for library feature detection"](/blog/2018/10/26/cpp-feature-macros/) (2018-10-26).

Every C++ implementation is required to provide feature-test macros indicating which
features it supports. These come in two flavors:

- Core-language macros such as `__cpp_generic_lambdas`,
    specified in [[cpp.predefined]](https://eel.is/c++draft/cpp.predefined#tab:cpp.predefined.ft).
    These are provided by the compiler vendor via the same mechanism as `__FUNCTION__` and `__GNUC__`.

- Library macros such as `__cpp_lib_ranges_iota`,
    specified in [[version.syn]](https://eel.is/c++draft/version.syn).
    These are provided by the library vendor via the same mechanism as `assert` and `_GLIBCXX_RELEASE`.
    They all begin with `__cpp_lib_`. The easiest way to get all of them at once is
    to `#include <version>` (since C++20).

Most [WG21](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#cwg-ewg-ewgi-lewg-lewgi-lwg) proposal papers
not only change the Standard in some way but also add a feature-test macro of the appropriate kind.
This lets the end-user C++ programmer detect whether their vendor claims to fully implement that feature yet.

---

I've always thought that the primary reason for feature-testing was _conditional preprocessing_.
We can write _polyfills_ like this:

    #include <version>
    #if __cpp_lib_expected >= 202211L
     #include <expected>
    #else
     #include <tl/expected.hpp>
     namespace std {
       using tl::expected;
       using tl::unexpected;
     }
    #endif

Or we can conditionally enable some functionality of our own library:

    template<class PairLike>
    auto extract_x_coord(const PairLike& p) {
    #if __cpp_structured_bindings >= 201606L
      const auto& [x, y] = p;
      return x;
    #else
      // We can still work for std::pair, at least
      return p.first;
    #endif
    }

This implies that if a paper does some very minor tweak, such that the code to detect-and-work-around
the absence of that feature would always cost more than simply avoiding the feature altogether, then
the paper doesn't really need an associated feature-test macro. Semi-example 1: We might think it's
"obvious" that the code above should instead be written like this, so that it's portable even to
compilers without structured bindings. This attitude would reduce the necessity for a feature-test macro:

    template<class PairLike>
    auto extract_x_coord(const PairLike& p) {
      static_assert(std::tuple_size<PairLike>::value == 2,
          "p must have exactly two parts");
      using std::get;
      return get<0>(p);
    }

Semi-example 2: We vacillated up to the last minute on whether
[P2447 "`std::span` over initializer list"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2447r6.html)
needed a feature-test macro or not, since we couldn't imagine anybody writing conditional code
like this:

    void f(std::span<const int>);
    int main() {
    #if __cpp_lib_span_initializer_list >= 202311L
      f({1,2,3});
    #else
      f({{1,2,3}});
    #endif
    }

However, in the end we did add that feature-test macro.

---

The other day, I heard a second plausible use-case for feature-test macros. This new use-case makes
it a good idea to add a macro for pretty much every paper, no matter how small. The new idea is that
you can write your code "at head," using whatever features of modern C++ you care to use, with no
`#if`s at all; and then you can just _assert to your build system_ that all the features you use are
in fact implemented by your current platform.

So we simply write:

    #include <expected>
    template<class PairLike>
    auto extract_x_coord(const PairLike& p) {
      const auto& [x,y] = p;
      return x;
    }
    void f(std::span<const int>);
    int main() {
      f({1,2,3});
    }

and then in a companion file (which could be a file built only by CMake, or a unit test,
or simply a header pulled into our build at any point) we assert the features we expect
to be available:

    #include <version>
    static_assert(__cpp_lib_expected >= 202211L);
    static_assert(__cpp_lib_span_initializer_list >= 202311L);
    static_assert(__cpp_structured_bindings >= 201606L);

If all these assertions pass, then the platform is "modern enough" for our codebase.
If any assertion fails, we simply tell the library-user to upgrade their C++ compiler
and we're done. No fallbacks, no polyfills, just a simple file full of features and
version numbers — just like a [pip `requirements.txt` file](https://pip.pypa.io/en/stable/reference/requirement-specifiers/)
or the dependencies in a [`package.json`](https://docs.npmjs.com/cli/v10/configuring-npm/package-json#dependencies).

In this model, there's no cost at all to "detecting the absence of a feature," because
we never intend to work around that absence. It's cheap to add a single line to the
"requirements header." So paper authors should be liberal in adding feature-test macros
to their proposals.

---

Once a feature-test macro has been added to the Standard it can never be removed,
by definition. Will the C++11-era system of named feature-test macros eventually
collapse under its own weight? Perhaps.

The C++11 system has already buckled in one sense: The original idea was that each macro's
value could just be _bumped_ every time a change was made to that feature. For example,
[`__cpp_constexpr` has been bumped eight times](https://en.cppreference.com/w/cpp/feature_test#Language_features)
as more and more of the core language has been made constexpr-friendly.
To take another relatively extreme example, `__cpp_lib_ranges`
[started](https://en.cppreference.com/w/cpp/feature_test#Library_features) at `201911L`,
then [P2415 `owning_view`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2415r2.html) bumped it to `202110L`,
then [P2602 "Poison Pills Are Too Toxic"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2602r2.html) bumped it to `202211L`,
and so on (omitting some intermediate bumps). But this system works only
if all vendors can be relied on to implement P2415 _before_ P2602, since there's no value of the macro
that would correspond to "I implement P2602 without P2415."
Many Ranges-relevant papers (e.g. [P1206 `ranges::to`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1206r7.pdf))
introduce their own macros different from `__cpp_lib_ranges`, to allow vendors to implement them
independently of any other changes to the same header.
Eventually, the good names might all be taken.

I can imagine someday simply naming macros after paper numbers,
e.g. `__cpp_lib_p1234r6`; but I think that day is still a long way off.
{% endraw %}
