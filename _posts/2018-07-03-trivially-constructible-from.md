---
layout: post
title: 'Trivially-constructible-from'
date: 2018-07-03 00:01:00 +0000
tags:
  rant
  relocatability
  standard-library-trivia
  triviality
---

[Today I learned:](https://godbolt.org/g/ZWGL9e)

    using u64 = long long;
    using u64b = long;
    static_assert(sizeof (u64) == 8);
    static_assert(sizeof (u64b) == 8);

    void foo(u64 *p, u64 *q, int n)
    {
        std::copy(p, p+n, q);
    }

    void bar(u64 *p, u64b *q, int n)
    {
        std::copy(p, p+n, q);
    }

Both Clang and GCC were smart enough to optimize `foo`
into 8 or 9 instructions ending with a tail-call to `memmove`.

`bar` generates 30 instructions on GCC; 82 on Clang.

It would be cool if library vendors could just check
`is_trivially_constructible_v<u64, u64b>` and optimize to
a `memmove` in that case. But we can't have nice things:

    assert(is_trivially_constructible_v<u64, u64b>);
    // Yay!

    using u16 = short;
    assert(is_trivially_constructible_v<u64, u16>);
    // What the...

    assert(is_trivially_constructible_v<u64, double>);
    // ...oh geez.

Yep, `is_trivially_constructible` returns always-true
for scalar types. Because the standard [says](http://eel.is/c++draft/meta.unary.prop)

> [the construction] is known to call no operation that is not trivial

where "trivial" is a concept that is never defined anywhere in the Standard
(except for special member functions, and `u64` doesn't have member functions).
Vendors have interpreted this as meaning that scalar types should *always*
count as "trivially constructible," because initializing a scalar type never
"calls" any operation (although it may still perform arbitrarily complex math).

This is why we can't have nice things.

----

Unrelatedly, GCC currently [believes](https://godbolt.org/g/SfcQiC)
that `is_trivially_constructible_v<int,void*> == true` and simultaneously
that `is_constructible_v<int,void*> == false`.
[This is a bug.](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86398)
