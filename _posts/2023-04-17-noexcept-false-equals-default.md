---
layout: post
title: "Trivial functions can still be non-nothrow (modulo compiler bugs)"
date: 2023-04-17 00:01:00 +0000
tags:
  exception-handling
  implementation-divergence
excerpt: |
  Caveat lector: This post deals with extremely non-practical compiler trivia.

  What happens when you `=default` a special member function so as to
  make it trivial, but also tell the compiler that it's `noexcept(false)`?
  The Standard permits this construct ([[dcl.fct.def.default]](https://eel.is/c++draft/dcl.fct.def.default#2.3)),
  but it doesn't explicitly say what happens to the defaulted function —
  is the resulting trivial function nothrow, or not?
---

Caveat lector: This post deals with extremely non-practical compiler trivia.

What happens when you `=default` a special member function so as to
make it trivial, but also tell the compiler that it's `noexcept(false)`?
The Standard permits this construct ([[dcl.fct.def.default]](https://eel.is/c++draft/dcl.fct.def.default#2.3)),
but it doesn't explicitly say what happens to the defaulted function —
is the resulting trivial function nothrow, or not?

I expect the resulting function to be both trivial and throwing
(as oxymoronic as that sounds). Clang agrees with my expectation
in all cases. GCC, MSVC, and EDG all agree that the resulting function
is trivial, but differ as to whether it's throwing (in ways that disagree
with each other). [Here's my test suite.](https://godbolt.org/z/x3jY9b3Mh)

---

    struct DC {
      explicit DC() noexcept(false) = default;
    };

Everyone agrees that `std::is_trivially_default_constructible_v<DC>`;
but GCC and EDG think it's also `std::is_nothrow_default_constructible_v<DC>`.
MSVC gives the right answer for the type traits, but joins GCC and EDG in
evaluating the core-language expression `noexcept(DC())` to `true` instead of `false`.
MSVC, GCC, and EDG also evaluate the noexceptness of `DC()` to `true` instead of `false`
when it's inside a `requires`-expression:

    template<class T, class... Args>
    concept NoexceptConstructible = requires {
      { T(std::declval<Args>()...) } noexcept;
    };
    static_assert(!NoexceptConstructible<DC>); // Clang only

This pattern repeats for the copy constructor and move constructor:
Clang does the right thing (in my opinion), MSVC gives the right answer for
the type traits but not for `noexcept`-expressions or `requires`-expressions,
and GCC and EDG give the wrong answer throughout.

---

For the destructor, Clang and EDG do the right thing; GCC does the wrong thing;
MSVC gives the right answer for the type traits but not for `noexcept`-expressions.

---

For the assignment operators, the situation is weirder ([Godbolt](https://godbolt.org/z/PPx4G4P9s)).
For GCC, EDG, and MSVC, the type traits and `noexcept`-expressions uniformly
(and wrongly) report that assignment is non-throwing. But if you ask for
`&T::operator=` as a noexcept function pointer, it (correctly) fails!

    struct CA {
      CA& operator=(const CA&) noexcept(false) = default;
    };
    CA& (CA::*mfpa)(const CA&) noexcept = &CA::operator=;
      // Error (all four compilers agree): operator= isn't noexcept
    static_assert(!noexcept(CA() = CA()));
      // And yet GCC+EDG+MSVC wrongly fail this assertion!

---

For `operator==` and `operator<=>`, everyone does the right thing.

For `operator!=`, `operator<`, `operator<=`, `operator>`, and `operator>=`,
Clang, GCC, and MSVC all do the right thing; EDG has the same weird behavior
as for the assignment operators ([Godbolt](https://godbolt.org/z/sG9TjYP4z)),
where `&T::operator<` is correctly non-convertible to a noexcept function pointer
type, but `noexcept(a < b)` incorrectly evaluates to `true`.

---

["Trivially-constructible-from"](/blog/2018/07/03/trivially-constructible-from/) (2018-07-03)
points out that a trivial conversion can do more than simply copy bits; for example,
[this](https://godbolt.org/z/s11oj4xro) conversion from `D&` to `B&` is considered
"trivial," but it doesn't just copy the bits; it generates an `add` instruction to
offset the `this` pointer.

    struct D : std::array<int, 25>, B {};

    B& plus100(D& d) {
        static_assert(std::is_trivially_constructible_v<B&, D&>);
        return d;
    }

However, I'm fairly confident that an expression consisting entirely of trivial operations
can never actually throw an exception at runtime. If you think of a way to do it,
please let me know!
