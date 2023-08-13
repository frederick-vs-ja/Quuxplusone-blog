---
layout: post
title: 'Constrained forwarding references considered sketchy as frick'
date: 2018-09-09 00:01:00 +0000
tags:
  concepts
  metaprogramming
  pitfalls
---

I'll just leave [this](https://godbolt.org/z/oSRvvp) here.

    template<class I> requires is_integral_v<I>
    void foo(I&& i) {
        puts("ONE");
    }

    template<class T> // unconstrained
    void foo(T&& t) {
        puts("TWO");
    }

The intent of this code (says the casual reader) is that `foo` takes
one parameter by forwarding reference; and if that parameter is integral,
it'll do one thing and otherwise it'll do the other thing. But watch:

    int main() {
        constexpr int zero = 0;
        foo(int(zero));
        foo(zero);
    }

This [prints](https://wandbox.org/permlink/yBKPKpCEwny8Go5L) "ONE TWO".

This just came up in the wild, so to speak,
[on the Code Review StackExchange](https://codereview.stackexchange.com/questions/203435/c-multithread-pool-class/).

----

UPDATE, 2023-08-13: This remains true for arbitrary constraints and concepts
like `std::integral`. But some C++20 concepts are meant to be used this way;
notably, C++20 Ranges expects you to write

    void foo(std::ranges::range auto&& rg) {
        use(rg);
    }

The trick in Ranges' case is that the concept is specially designed to ignore
ref-qualification and Do The Right Thing with const-qualification; in Ranges'
case this is part of a constellation of decisions, such as that taking by
`const range auto& rg` [would be wrong](/blog/2023/08/13/non-const-iterable-ranges/)
and that you are going to treat `rg` as an lvalue instead of perfect-forwarding it along.
