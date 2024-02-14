---
layout: post
title: 'The `Auto` macro'
date: 2018-08-11 00:01:00 +0000
tags:
  c++-style
  lambdas
  memes
  pearls
  preprocessor
---

I've been talking about this one [since 2014](https://www.youtube.com/watch?v=lKG1m2NkANM).
The contents of "Auto.h" fit comfortably on a single slide:

    #pragma once

    template<class L>
    class AtScopeExit {
        L& m_lambda;
    public:
        AtScopeExit(L& action) : m_lambda(action) {}
        ~AtScopeExit() noexcept(false) { m_lambda(); }
    };

    #define TOKEN_PASTEx(x, y) x ## y
    #define TOKEN_PASTE(x, y) TOKEN_PASTEx(x, y)

    #define Auto_INTERNAL1(lname, aname, ...) \
        auto lname = [&]() { __VA_ARGS__; }; \
        AtScopeExit<decltype(lname)> aname(lname);

    #define Auto_INTERNAL2(ctr, ...) \
        Auto_INTERNAL1(TOKEN_PASTE(Auto_func_, ctr), \
            TOKEN_PASTE(Auto_instance_, ctr), __VA_ARGS__)

    #define Auto(...) \
        Auto_INTERNAL2(__COUNTER__, __VA_ARGS__)

Wrap any arbitrary amount of code in `Auto(...)` to generate a hygienic "scope guard."

    bool Mutate(State *state)
    {
        state->DisableLogging();
        Auto(state->EnableLogging());

        if (!state->AttemptOperation1()) return false;
        if (!state->AttemptOperation2()) return false;
        return true;
    }

This scope guard has [perfect codegen with zero overhead](https://godbolt.org/z/szqfxzq6j)
(at `-O2`) on all major compilers. Because it requires no explicit captures, no novel
variable name, and no special treatment for `this`, it is highly suited for
mechanically generated code. It is portable all the way back to C++11.

[It even nests!](https://godbolt.org/z/bsbvT6Tz5)

    void Example()
    {
        Auto(
            puts("starting the first Auto");
            Auto(puts("cleaning up in the second Auto"));
            puts("getting ready to clean up in the first Auto");
        );
        puts("doing the main operation");
    }

![I don't always use ad-hoc scope guards. But when I do, I prefer "Auto.h".](/blog/images/2018-08-11-ad-hoc-scope-guards.jpg)

See also:

* ["Feature requests for the `Auto` macro"](/blog/2024/02/14/auto-macro-feature-requests/) (2024-02-14)
