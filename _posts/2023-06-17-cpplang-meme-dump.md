---
layout: post
title: "A small C++ meme dump"
date: 2023-06-17 00:01:00 +0000
tags:
  concepts
  cpplang-slack
  memes
  relocatability
  stl-classic
  wg21
excerpt: |
  A small dump of C++ memes, starting with two fresh off the grill from the Varna WG21 meeting.
---

![](/blog/images/2023-06-17-inplace-vector-argument.jpg){: .meme}

[P0843 "`inplace_vector`"](https://isocpp.org/files/papers/P0843R8.html#Move-semantics) (f.k.a. `static_vector`, `fixed_capacity_vector`);
[P1144R8 §2.1.3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r8.html#benefit-fixed-capacity).
See ["P1144 case study: Moving a `fixed_capacity_vector`"](/blog/2019/02/22/p1144-fixed-capacity-vector/) (2019-02-22).

`std::pmr::vector` either clears itself after move or not, depending on library vendor and
most importantly depending on whether the destination has the same arena. Neither criterion
relates to `T`'s own triviality. ([Godbolt.](https://godbolt.org/z/PabzGMY4h))
See also ["A not-so-quick introduction to the C++ allocator model"](/blog/2023/06/02/not-so-quick-pmr/) (2023-06-02)

----

![](/blog/images/2023-06-17-scalblnf.jpg){: .meme}

The math library for some reason has [both](https://en.cppreference.com/w/cpp/numeric/math/scalbn)
`scalbn(double, int)` and `scalbln(double, long int)`. Both functions simply add the int to the exponent
of the floating-point number (scaling its real value by $$2^n$$). For C compatibility, they also come
in `float` flavor (`scalbnf`, `scalblnf`) and `long double` flavor (`scalbnl`, `scalblnl`).
Presumably `scalblnf` is useful on platforms where the number of bits in a `float`'s
[exponent field](/blog/2021/09/05/float-format/) (e.g. 8) is greater than the number of bits in an `int` (e.g. 32).

Cppreference describes `scalblnf` and friends under the drippingly sarcastic heading
["Common mathematical functions."](https://en.cppreference.com/w/cpp/numeric/math)

----

![](/blog/images/2023-06-17-but-i-like-this.jpg){: .meme}

I don't even know which way around this meme is supposed to go. `a.merge(b)` does destructively modify `b`
(in that it pilfers all its nodes over into `a`), so you might think `a.merge(std::move(b))` correctly
indicates that this had better be the last use of `b`. On the other hand, it reliably leaves `b` empty,
not in some unspecified moved-from state — in fact, [`set::merge`](https://en.cppreference.com/w/cpp/container/set/merge)
leaves its right-hand argument in a reliably _useful_ state! — and so the use of `std::move` might be
philosophically incorrect. Certainly it's more tedious to type out. I don't know, man.

----

![](/blog/images/2023-06-17-you-wouldnt-forward-declare-a-concept.png){: .meme}

Sometimes people ask, if you can forward-declare a class template or a function template, why can't
you also forward-declare a concept?

    template<class T> int f(T t); // OK
    template<class T> class Mine; // OK
    template<class T> concept Fungible; // ill-formed

The answer is that a function or class is usable without its definition: the compiler can deal with `i = f(42)`
or `Mine<int> *p = nullptr` without knowing anything else about `f` or `Mine`. But to "use" a concept, you need
its definition.

    template<class U> int g(U u) requires Fungible<U>;

Is `g(42)` a valid function call? We don't know unless we know the value of `Fungible<int>`, which requires
seeing the definition of `Fungible`. Forwarding-declaring `concept Fungible` is useless. Therefore it's
not allowed.

----

![](/blog/images/2023-06-17-my-concepts-look-like-this.jpg){: .meme}

If you find yourself confused about the syntax of requires-expressions, or can't think of how
to write a concept testing for a particular property, just remember that requires-expressions are
designed so that this works. (Including the sneaky behavior of `Bar&&` above!)
Contrariwise, if you find yourself writing a _lot_ of concepts that look like this meme,
consider maybe not writing those concepts at all: They likely aren't doing anything except
to change the source location associated with your error messages.
