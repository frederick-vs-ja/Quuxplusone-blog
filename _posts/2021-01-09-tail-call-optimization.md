---
layout: post
title: "It's not always obvious when tail-call optimization is allowed"
date: 2021-01-09 00:01:00 +0000
tags:
  c++-learner-track
  implementation-divergence
---

I initially wrote this as part of a new entry in
["A C++ acronym glossary"](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/) (2019-08-02),
but decided that it would be better to pull it out into its own post.

"TCO" stands for "tail call optimization."
This is a compiler optimization that takes what appears in the source code
as a function call (which usually pushes an activation frame onto the call stack) and turns
it into a plain old jump (which does not push a frame on the call stack). This is possible only
when the function call appears at the very tail end of the function — something like `return bar()`.
The compiler is saying, "Look, I know `bar` is going to end by returning to its caller; that's just
me, and I have nothing left to do. So let's just trick `bar` into returning to _my_ caller!"

Tail call optimization is often possible for `return bar()` but not for, e.g., `return bar()+1`.

In C++, it can be very hard for a human to figure out exactly where TCO is allowed to happen.
The main reason is non-trivial destructors:

    void foo1() { bar(); }  // tail call
    void foo2() { std::lock_guard lk(m); bar(); }  // not a tail call

including destructors of temporary objects:

    void bar(std::string_view);
    void foo1() { bar("hello"); }  // tail call
    void foo2() { bar("hello"s); }  // not a tail call

but even when everything is trivially destructible, you might need to adjust the stack
pointer or something, thus preventing TCO. [Godbolt](https://godbolt.org/z/vcY3v9):

    void bar();
    void escape(const int&);
    void foo1() { escape(42); bar(); }  // tail-call on GCC and MSVC
    void foo2() { const int i = 42; escape(i); bar(); }  // not a tail-call

Interestingly, in the above example, GCC and MSVC emit `jmp bar` for `foo1`, but
Clang and ICC miss that optimization.

You might reasonably ask why we can't do the
same optimization for `foo2`. I think the reason is that C++ guarantees that every
variable (within its lifetime) has a unique address.
If we were to implement `bar` like this:

    const int *addr_of_i;
    void escape(const int& i) {
        addr_of_i = &i;
    }
    void bar() {
        int j;
        assert(&j != addr_of_i);
    }

then the program could tell whether the implementation had (non-conformingly)
put `j` into the same memory location as `i`. However, there's no rule that says
`j` can't share a memory location with the temporary object produced from `42`,
since that temporary's lifetime doesn't overlap with `j`'s.

----

Whether an implementation does TCO is kinda-sorta observable, in the sense that
a tail-recursive function might use O(1) stack space with TCO but O(n) without TCO — thus
"blowing the stack" when the recursion goes deep enough. However, C++'s abstract machine
doesn't really have any notion of blowing the stack. There's no conforming way for a C++
program to detect that condition or deal with it.

Sufficiently paranoid C++ code will therefore avoid very deep recursion.
One technique for doing this is to do tail-recursion optimization by hand:

    int gcd(int x, int y) {
        if (x == 0) return y;
        return gcd(y % x, x);
    }

becomes

    int gcd(int x, int y) {
    top:
        if (x == 0) return y;
        std::tie(x, y) = std::tuple(y % x, x);
        goto top;
    }

which in turn becomes

    int gcd(int x, int y) {
        while (x != 0) {
            std::tie(x, y) = std::tuple(y % x, x);
        }
        return y;
    }

It's often pragmatically important to do this last step, not just because
[structured programming](https://en.wikipedia.org/wiki/Structured_programming)
makes the code easier for humans to understand,
but also because `goto` is one of the very few C++ constructs that
[prevents](https://stackoverflow.com/questions/45266577/why-disallow-goto-in-constexpr-functions)
marking the function as `constexpr`. If you want your function
to be constexpr (for whatever reason), you _must_ avoid `goto`.
This is a rare case of C++ being stylistically opinionated.

(If that `tie = tuple` trick is new to you, you might enjoy
my CppCon 2020 talk ["Back to Basics: Algebraic Data Types."](https://www.youtube.com/watch?v=OJzmWqCCZaM).)

----

See also:

* ["Type-erased `InplaceUniquePrintable` benefits from `noexcept`"](/blog/2022/07/30/type-erased-inplace-printable/) (2022-07-30)
