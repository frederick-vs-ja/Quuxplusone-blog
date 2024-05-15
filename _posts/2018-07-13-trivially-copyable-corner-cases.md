---
layout: post
title: "When is a trivially copyable object not trivially copyable?"
date: 2018-07-13 00:02:00 +0000
tags:
  pitfalls
  proposal
  relocatability
  triviality
  wg21
---

Subtitled, "The Itanium ABI ruins everything, again."

Answer number one: A trivially copyable object is not trivially copyable
when it is `volatile`. [Godbolt](https://godbolt.org/g/5hJyvB):

    struct S {
        volatile int i;
    };
    static_assert(std::is_trivially_copyable_v<S>);

    void foo(S *dst, S *src, int n) {
        std::copy_n(src, n, dst);
    }

Both libc++ and libstdc++ have implementations of `std::copy_n` that
optimize volatile loads and stores into `memcpy`, which causes tearing
of reads and writes.

I have written a draft proposal
[EDIT 2019-04-13: published as
[P1153R0 "Copying volatile subobjects is not trivial"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1153r0.html)]
— coauthored with JF Bastien, and seeking as many coauthors as possible! — 
to solve this problem once and for all.

----

Answer number two: A trivially copyable object is not trivially copyable
when it is a *potentially overlapping subobject*.
[Wandbox](https://wandbox.org/permlink/rE5xJ31L63DEj4bL):

    struct A { int a; };
    struct B : A { char b; };
    struct C : B { short c; };

    static_assert(!std::is_standard_layout<B>::value, "");
    static_assert(std::is_trivially_copyable<B>::value, "");

    int main()
    {
        C c1 { 1, 2, 3 };
        B& dst = c1;
        const B& src = C{ 5, 6, 7 };

        printf("before operator=: %d\n", int(c1.c));  // 3
        dst = src;
        printf("after operator=: %d\n", int(c1.c));  // 3

        printf("before std::copy: %d\n", int(c1.c));  // 3
        std::copy_n(&src, 1, &dst);
        printf("after std::copy: %d\n", int(c1.c));  // 7
    }

I'm not yet sure what is the right fix for this issue at the WG21 level,
but I believe it deserves to be fixed somehow.
