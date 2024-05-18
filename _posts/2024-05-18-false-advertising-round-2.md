---
layout: post
title: "Types that falsely advertise P2786 trivial relocatability"
date: 2024-05-18 00:01:00 +0000
tags:
  proposal
  relocatability
  st-louis-2024
  triviality
---

My previous blog post — ["Types that falsely advertise trivial copyability"](/blog/2024/05/15/false-advertising/) (2024-05-15) —
refers to my as-yet-pretty-vague proposal [P3279 "What 'trivially fooable' should mean"](https://isocpp.org/files/papers/P3279R0.html),
where I basically propose that `is_trivially_constructible<T, U>` should be true if and only if `T(declval<U>())` selects
a constructor or conversion operator which is "known to be equivalent in its observable effects to a simple copy of the
complete object representation." This is intended to be similar to the existing wording for defaulted special members, e.g.

> A copy/move constructor for class `X` is *trivial* if it is not user-provided and [...]
>
> - the constructor selected to copy/move each direct base class subobject is trivial, and
>
> - for each non-static data member of `X` that is of class type (or array thereof), the constructor selected to copy/move that member is trivial.

and to the proposed wording from [P2786R5](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2786r5.pdf):

> A class `C` is a *trivially relocatable class* if [...]
>
> - when an object of type `C` is direct-initialized from an xvalue of type `C`, overload resolution would select
>     a constructor that is neither user-provided nor deleted

I knew Corentin Jabot had implemented P2786R5 in a fork of Clang, so I went looking to see how he'd handled that
particular wording. It involves overload resolution, which would also be needed by my P3279's proposed new wording for
"trivially copyable," so I figured I should go see how that kind of thing is done in Clang.

Instead, I found no overload resolution: just a casewise check for a move-constructor, or if not a move-constructor then
a copy-constructor. That's not what P2786 says to do!

## A type mishandled by the P2786 reference implementation

[Godbolt](https://godbolt.org/z/4qM6E5znq):

    struct C {
      C(const C&) = default;
      template<class=void> C(C&&);
      C& operator=(C&&);
    };

When an object of type `C` is direct-initialized from an xvalue of type `C`, overload resolution
selects the constructor `C::C<void>(C&&)`, which is absolutely user-provided. So this type `C` shouldn't
be P2786-trivially-relocatable; but Corentin's reference implementation wrongly considers it to be.


## A type that falsely advertises P2786 trivial relocatability

[Godbolt](https://godbolt.org/z/69xrsj6vq):

    struct S {
      S(const volatile S&);
      template<class=void> S(const S&);
      S(S&&) = default;
    };
    static_assert(!__is_trivially_copyable(S));
    static_assert(!__is_trivially_relocatable(S));
    static_assert(__is_trivially_constructible(S, S&&));
    #ifdef P2786
      static_assert(__is_cpp_trivially_relocatable(S));
    #endif

    struct T { S s; T(const T&) = default; };
    static_assert(!__is_trivially_copyable(T));
    static_assert(!__is_trivially_relocatable(T));
    static_assert(!__is_trivially_constructible(T, T&&));
    #ifdef P2786
      static_assert(__is_cpp_trivially_relocatable(T));
    #endif

This corresponds to my previous post's [`Plum` example](/blog/2024/05/15/false-advertising/#a-type-that-falsely-advertises-trivial-copyability).
The core language has one idea of how to relocate `T` — i.e., call its non-trivial copy constructor
followed by its destructor:

    void simple_correct_relocate(T *s, T *d) {
      ::new(d) T(std::move(*s));
      s->~T();
    }

— and P1144's `std::relocate_at` rightly follows the core language, since `T` is not in fact
trivially relocatable. But P2786R5 treats `is_trivially_relocatable<T>` as true (because overload
resolution on a move-construction of `T` selects `T`'s defaulted copy constructor, and we don't
look any deeper to see what that copy constructor actually does with `T`'s `S` data member).

Corentin's P2786 reference implementation follows P2786's dictates and bypasses `T`'s non-trivial relocation operation:

    void new_relocate(T *s, T *d) {
      static_assert(__is_cpp_trivially_relocatable(T));
      std::trivially_relocate(s, s+1, d);
    }
    new_relocate(T*, T*): # @new_relocate(T*, T*)
      movzbl (%rdi), %eax
      movb %al, (%rsi)
      retq

My P1144 implementation rightly gives the same code as `simple_correct_relocate`:

    void new_relocate(T *s, T *d) {
      static_assert(!__is_trivially_relocatable(T));
      std::relocate_at(s, d);
    }
    new_relocate(T*, T*): # @new_relocate(T*, T*)
      movq %rdi, %rax
      movq %rsi, %rdi
      movq %rax, %rsi
      jmp S::S<void>(S const&)@PLT # TAILCALL


## A type that falsely advertises both P1144 and P2786 trivial relocatability

[Godbolt](https://godbolt.org/z/54ExfdE4e):

    struct S {
      S(const volatile S&) = delete;
      template<class=void> S(const S&);
      S(S&&) = default;
    };
    static_assert(__is_trivially_copyable(S));
    static_assert(__is_trivially_relocatable(S));
    static_assert(__is_trivially_constructible(S, S&&));
    #ifdef P2786
      static_assert(__is_cpp_trivially_relocatable(S));
    #endif

    struct T { S s; T(const T&) = default; };
    static_assert(!__is_trivially_copyable(T));
    static_assert(!__is_trivially_constructible(T, T&&));
    #ifdef P2786
      static_assert(!__is_trivially_relocatable(T));
      static_assert(__is_cpp_trivially_relocatable(T));
    #else
      static_assert(__is_trivially_relocatable(T));
    #endif

In a post-P3279 world, `is_trivially_copyable` would depend on the results of overload resolution
instead of just looking at special members. So `S` would not be considered `is_trivially_copyable`,
and therefore it wouldn't be P1144-trivially-relocatable; and therefore neither would `T`.
