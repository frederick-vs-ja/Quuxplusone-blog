---
layout: post
title: "PSA: Value-initialization is not merely default-construction"
date: 2023-06-22 00:01:00 +0000
tags:
  initialization
  language-design
  metaprogramming
  proposal
  varna-2023
excerpt: |
  At the Varna WG21 meeting, Giuseppe D'Angelo presented his
  [P2782 "Type trait to detect if value initialization can be achieved by zero-filling"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2782r0.html).
  The intent of this new type trait is to tell `vector`'s implementor
  whether

      std::vector<T> v;
      v.resize(1000);

  can just do a `memset` of all the `T` objects to put them into their correctly
  constructed state. By "correctly constructed state," I mean the state a `T` would
  be in after

      ::new (p) T(); // value-initialization
---

At the Varna WG21 meeting, Giuseppe D'Angelo presented his
[P2782 "Type trait to detect if value initialization can be achieved by zero-filling"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2782r0.html).
The intent of this new type trait is to tell `vector`'s implementor
whether

    std::vector<T> v;
    v.resize(1000);

can just do a `memset` of all the `T` objects to put them into their correctly
constructed state. By "correctly constructed state," I mean the state a `T` would
be in after

    ::new (p) T(); // value-initialization

The syntax `T()` [causes](http://eel.is/c++draft/dcl.init.general#16.4.sentence-1)
a kind of initialization called [_value-initialization_](http://eel.is/c++draft/dcl.init.general#9).
Here's a public service announcement (not for Giuseppe, who knows;
but perhaps for you, dear reader; and probably for me again six months from now) —

> Value-initialization is not merely default-construction!

Value-initialization for a type with a defaulted default constructor
does zero-initialization first, then calls the default constructor.
The effects are most visible for primitive scalar types:

    int i;          // default-initialized, garbage value
    int j = int();  // value-initialized, always zero

`int` is both trivially default-constructible (because you can default-initialize
an `int` by default-initializing each of the unsigned chars in its
[_object representation_](https://eel.is/c++draft/basic.types#general-4.sentence-1) to garbage),
and trivially value-initializable (because you can value-initialize an `int` by
value-initializing each of the unsigned chars in its object representation to zero).

Yet "trivially default-constructible" and "trivially value-initializable" are 100% orthogonal
properties. Here's a constructive proof:

    struct T;
    struct S1 {
        explicit S1() = default;
        int T::*mf;
    };
    struct S2 {
        explicit S2() {}
        int T::*mf;
    };

Struct `S1` is trivially default-constructible, but it is not trivially value-initializable.
Struct `S2` is trivially value-initializable, but it is not trivially default-constructible.

Lacking a compiler builtin to query trivial value-initializability, we prove our assertion
empirically by writing a little function and seeing how the compiler optimizes it.

    S1 f1() { return S1(); }
    S2 f2() { return S2(); }

`clang++ -O2` [produces](https://godbolt.org/z/jKWjG3x1x):

    _Z2f1v:
      movq $-1, %rax
      retq
    _Z2f2v:
      retq

The object representation of a value-initialized `S1()` is all-bits-one, but the object representation
of a value-initialized `S2()` is not just all-bits-zero — it's actually indeterminate! We might call that
"even better than trivial."

> To focus on "better than triviality" would be a mistake: only contrived types like `S2`
> partake of it. In fact we would expect `__is_trivially_value_initializable(S2)` to report `false`
> in practice, because `S2`'s default constructor is user-provided and thus not readily inspectable
> by the front-end. Its triviality is discovered only by the optimizer.
