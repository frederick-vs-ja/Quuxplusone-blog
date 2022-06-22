---
layout: post
title: 'The abstraction penalty for wide integer math on x86-64'
date: 2020-02-13 00:01:00 +0000
tags:
  benchmarks
  llvm
  sufficiently-smart-compiler
---

Back in November 2018, following
[a thread on the std-proposals mailing list](https://groups.google.com/a/isocpp.org/forum/#!topic/std-proposals/SlNHpw096IM),
I sat down and wrote out [a really simple "wide integer" library](https://github.com/Quuxplusone/WideIntProofOfConcept).

"Wide integers" are integers of fixed-but-bigger-than-word-length size.
For example, the `__uint128_t` supported by GCC and Clang would be
considered a "wide" integer.
Wide integers are not to be confused with "big integers," i.e., integers of
dynamic (heap-allocated) size.

I can also imagine "wide floating-point," "wide fixed-point," "wide rational"
(that is, a number expressed and stored as the *ratio* of two wide integers),
and "big rational." I doubt that "big fixed-point" or "big floating-point"
make philosophical sense.

----

My library provides a single class template: `Wider<T>`.
The `T` can be either `uint64_t`, or some specialization of `Wider`.
Each `Wider` doubles the bit-width of the unsigned integer represented;
so we can say

    using Uint128 = Wider<uint64_t>;
    using Uint256 = Wider<Uint128>;
    using Uint512 = Wider<Uint256>;
    using Uint1024 = Wider<Uint512>;

I looked at Clang's codegen for `__uint128_t` to see what might be the most optimal
codegen, and then tweaked my code to try to achieve the same codegen. It was
actually pretty easy, on Clang/LLVM! On GCC, not so much.

The idea is that we can write plain old C++ code — maybe with a few intrinsics to
help out at the lowest level — and then let the compiler do all the hard work of
optimizing it. Here's the code for wide-integer addition:

    inline bool producecarry(uint64_t& x, uint64_t y) {
        x += y;
        return (x < y);
    }

    inline bool addcarry(bool cf, uint64_t& x, uint64_t y) {
        return _addcarry_u64(cf, x, y, (unsigned long long*)&x);
    }

    template<class Int64>
    struct Wider {
        Int64 lo;
        Int64 hi;

        friend bool producecarry(Wider& x, const Wider& y) {
            return addcarry(producecarry(x.lo, y.lo), x.hi, y.hi);
        }

        friend bool addcarry(bool cf, Wider& x, const Wider& y) {
            cf = addcarry(cf, x.lo, y.lo);
            cf = addcarry(cf, x.hi, y.hi);
            return cf;
        }

        friend Wider& operator+=(Wider& x, const Wider& y) { (void)producecarry(x, y); return x; }
        friend Wider operator+(Wider x, const Wider& y) { x += y; return x; }
    };

[Compile this with Clang trunk and we see](https://godbolt.org/z/TAANe_) that
the compiler produces the exact same code for

    Uint128 example(const Uint128& x, const Uint128& y)
    {
        return x + y;
    }

    __uint128_t example(const __uint128_t& x, const __uint128_t& y)
    {
        return x + y;
    }

And that code is optimal:

    movq (%rsi), %rax
    addq (%rdi), %rax
    movq 8(%rsi), %rdx
    adcq 8(%rdi), %rdx
    retq

[GCC trunk, on the other hand, performs abysmally.](https://godbolt.org/z/qYF3WN)

----

Anywhere there's a gap between the performance of `Wider<T>` and the performance of
a built-in type like `__uint128_t`, that's an opportunity for some compiler writer
to go improve the codegen. In this way, `WideIntProofOfConcept` is kind of like
the ["Stepanov Abstraction Penalty Benchmark"](http://www.open-std.org/jtc1/sc22/wg21/docs/D_3.cpp);
it points to places where the compiler could do better at recognizing peephole-level idioms.

Back in 2018–2019 I filed a bunch of bugs against LLVM inspired by this benchmark:
[39968](https://bugs.llvm.org/show_bug.cgi?id=39968),
[40090](https://bugs.llvm.org/show_bug.cgi?id=40090),
[24545](https://bugs.llvm.org/show_bug.cgi?id=24545),
[31754](https://bugs.llvm.org/show_bug.cgi?id=31754),
[40486](https://bugs.llvm.org/show_bug.cgi?id=40486),
[40825](https://bugs.llvm.org/show_bug.cgi?id=40825).
They were all fixed and closed quite expeditiously!

Bug [40908](https://bugs.llvm.org/show_bug.cgi?id=40908), an ICE on using `__int128` as an NTTP,
I actually discovered while writing ["Is `__int128` integral? A survey"](/blog/2019/02/28/is-int128-integral/)
(2019-02-28). It remains open.

To see the current "penalties" for using `Wider<T>` versus the built-in types,
see [the README on GitHub](https://github.com/Quuxplusone/WideIntProofOfConcept).
There's also a Python script you can download to regenerate
the table using Godbolt's API, if you want to try other (Godbolt-supported) compilers,
or just to see whether my numbers are up-to-date.

----

Finally, thanks to Niall Douglas for (two years ago) motivating this whole thing by pointing
out how absymally awful Boost.Multiprecision is! [Here is](https://gcc.godbolt.org/z/mrFXgG)
a reduced version of Niall's benchmark for `a + b`, showing `boost::multiprecision::uint128_t`
alongside native `unsigned __int128`; `Wider<uint64_t>`; and the new kid on the block,
Abseil's `absl::uint128`.

**Abseil's `absl::uint128` seems to be high-quality.** I do detect some places where it gives worse codegen
than `Wider<uint64_t>` — for example, [try the expression `a + -b`](https://gcc.godbolt.org/z/q46GKv) —
but ye gods, Abseil is _worlds_ better than Boost.Multiprecision!
