---
layout: post
title: 'Exploring C++ types with `puts(__PRETTY_FUNCTION__)`'
date: 2018-08-22 00:01:00 +0000
tags:
  msvc
  pearls
  templates
---

Today on Slack I noticed that some people still don't know this one, so it should be
shared more widely.

    template<class T>
    void f() {
        puts(__PRETTY_FUNCTION__);
    }

You can use this magic formula to explore what the compiler is doing to your types,
either for metaprogramming purposes or just to learn more about `auto`, `auto&&`,
and `decltype(auto)`. [For example:](https://wandbox.org/permlink/4fmL5MEjCC3jmfEA)

    #define EXPLORE(expr) \
        printf("decltype(" #expr ") is... "); \
        f<decltype(expr)>();

    int main() {
        auto x = 5;
        auto&& y = 5;
        decltype(auto) z = 5;

        EXPLORE(x); EXPLORE((x));
        EXPLORE(y); EXPLORE((y));
        EXPLORE(z); EXPLORE((z));
    }

The MSVC (Visual Studio) equivalent of `__PRETTY_FUNCTION__` is `__FUNCSIG__`.

Sample GCC/EDG `__PRETTY_FUNCTION__`:

    decltype(x) is... void f() [with T = int]
    decltype((x)) is... void f() [with T = int &]

Sample Clang `__PRETTY_FUNCTION__`:

    decltype(x) is... void f() [T = int]
    decltype((x)) is... void f() [T = int &]

Sample MSVC `__FUNCSIG__`:

    decltype(x) is... void __cdecl f<int>(void)
    decltype((x)) is... void __cdecl f<int&>(void)
