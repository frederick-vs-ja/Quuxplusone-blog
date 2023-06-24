---
layout: post
title: "Polymorphic types aren't trivially relocatable"
date: 2023-06-24 00:01:00 +0000
tags:
  classical-polymorphism
  proposal
  relocatability
  varna-2023
---

One of the (minor, non-key) differences between [P1144](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r8.html#wording-basic.types.general)
trivial relocatability and [P2786R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2786r1.pdf)
trivial relocatability (§11.3, page 19) is that P2786R1 accidentally considers polymorphic types
to be trivially relocatable by default. I say "accidentally" because of course this cannot _really_
be permitted. [Godbolt](https://godbolt.org/z/5vTnWxEMa):

    struct B {
        virtual int getter() const { return 1; }
        virtual ~B() = default;
    };
    struct D : B {
        int d = 2;
        int getter() const override { return d; }
    };

    alignas(D) char dbuf[sizeof(D)];
    alignas(B) char bbuf[sizeof(B)];
    B *d = ::new (dbuf) D();
    B *b = (B*)bbuf;
    // RELOCATE
    printf("Expect 1: %d\n", b->getter());
    b->~B();

The "`RELOCATE`" comment should be replaced with the code to relocate a `B` object from `*d` into `*b`.
In P1144, the spelling is simply

    std::relocate_at(d, b);

and the library decides whether to use trivial relocation behind the scenes. In P2786R1, the spelling is

    static_assert(std::is_trivially_relocatable_v<B>);
    std::trivially_relocate(d, d+1, b);

In P1144, `B` is not considered trivially relocatable, and the code above Just Works.

> Why is `B` not trivially relocatable? P1144 offers two answers: One, at the compiler level, `B` doesn't meet
> [[basic.types.general]](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r8.html#wording-basic.types.general)'s criterion
> that a trivially relocatable type mustn't have virtual member functions. Two, at the human level, we
> can see that `B` fails to model [[concept.relocatable]](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r8.html#wording-concept.relocatable)'s
> semantic requirements.

In P2786R1, `B` is ("accidentally") considered trivially relocatable, and the code above has undefined behavior
at runtime.

> `*d`'s vptr is memmoved into `*b`, and then the virtual call `b->getter()` invokes `D::getter`
> through that (now-erroneous) vptr, which tries to access the data member `d` of a `D` object
> that doesn't actually exist.

The Godbolt above uses my own Clang/libc++ implementation of P1144R8, and Corentin Jabot's
Clang implementation of P2786R0 (with none of the library components). I've told Corentin he
should feel free to adopt any pieces of my libc++ implementation that would help get the
P2786 demonstration into a working state. Of course, the authors of P2786 could also help
by refactoring P2786's library clauses to match P1144's. :)  As you can see, the P1144 library
facilities tend to be more usable than P2786's.

This might be a good time to remind folks that my Clang/libc++ implementation also provides
a `[[clang::maybe_trivially_relocatable]]` attribute implementing P2786's
["dull knife"](/blog/2023/03/10/sharp-knife-dull-knife/) semantics (but without P2786R1's
sharp-when-you-didn't-want-it parts, like its treatment of polymorphic types). So you can experiment
with a full libc++ implementation of dull-knife semantics simply by passing
`-D_LIBCPP_TRIVIALLY_RELOCATABLE_IF(x)=[[clang::maybe_trivially_relocatable(x)]]`.
([Godbolt.](https://godbolt.org/z/8Gj7K55vM))
