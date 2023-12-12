---
layout: post
title: "How to type-pun via catching by non-const reference"
date: 2023-11-17 00:01:00 +0000
tags:
  exception-handling
  implementation-divergence
excerpt: |
  I'll just leave this here ([Godbolt](https://godbolt.org/z/xf4Ka14Eo)):

      int i = 0x4048f5c3;
      try {
          try {
              throw (float*)nullptr;
          } catch (void *& pv) {
              pv = &i;
              throw;
          }
      } catch (const float *pf) {
          printf("%.2f\n", *pf);
      }
---

I'll just leave this here ([Godbolt](https://godbolt.org/z/xf4Ka14Eo)):

    int i = 0x4048f5c3;
    try {
        try {
            throw (float*)nullptr;
        } catch (void *& pv) {
            pv = &i;
            throw;
        }
    } catch (const float *pf) {
        printf("%.2f\n", *pf);
    }

On MSVC and Clang, this prints `3.14`.
On GCC (which I think means "on the libsupc++ runtime"), `pv` somehow ends up referring to a copy of
the original exception object, such that modifications to `pv` don't affect the subsequently caught
value of `pf`.

As far as I can tell, none of these three runtimes is doing the right thing. According to
[[except.handle]/3](https://eel.is/c++draft/except.handle#3), if I throw a type `E` that is
convertible to `T` only via some pointer conversion (e.g. `E`=`float*` to `T`=`void*`), I'm
actually not supposed to be able to hit a catch-handler of type `T&` at all — I'm supposed to
fly right past it! So (unless I'm mistaken) this is a "hole in the type-system" of all three
vendors' C++ runtimes, but it's not a hole in the paper standard, because the paper standard
says you're not supposed to be able to do this.

> UPDATE, 2023-12-12: In [GCC bug 23257](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=23257),
> it's stated that this isn't the fault of the compiler or the runtime library, but a flaw in
> the Itanium C++ ABI itself — the ABI's [`.gcc_except_table` section](https://maskray.me/blog/2020-12-12-c++-exception-handling-abi)
> encodes types in terms of `typeid`, which is identical for `T`, `T&`, and `const T&`.
> Notice the identical codegen in [this Godbolt](https://godbolt.org/z/3EYzKTG8E).
> Apparently [Arm's C++ ABI](https://github.com/ARM-software/abi-aa/blob/2a70c42/ehabi32/ehabi32.rst#personality-routine-helper-functions)
> can handle references differently from values-and-const-references, although personally
> I'm fuzzy on how that works and whether any mainstream compiler implements that ABI
> even on Arm targets. I don't know whether the MSVC issue is an ABI issue too, but I'd bet
> that it is.

----

This seems to have severe implications for [P2927 `try_cast`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2927r0.html).
[Last week I said](/blog/2023/11/12/kona-trip-report/#i-was-pleased-to-see-gor-nishano)
that it was always fine to take the `const T*` you got from the
cast and `const_cast` away the constness — the underlying exception object will never be
const. That's true of the underlying exception object, but the pointer you get back might
not be pointing to the underlying exception object! — it might be pointing to a temporary
`T` implicitly-converted-by-a-pointer-conversion from the actual exception object (whose
type is not `T` but `E`). In fact, if `try_cast`'s result points to a temporary, it'll be
dangling already! I guess we'll have to do something about this.

    auto e = std::make_exception_ptr<int*>(&i);
    auto *p = std::try_cast<void*>(e);
      // as-if-by catch (void* const&); a temporary is created
    // here p is a dangling pointer

> UPDATE, 2023-12-12: My proposed solution is simply to make `std::try_cast<T*>` ill-formed.
> Throwing pointers is already sketchy (and, as we've seen, buggy in the core language on
> all mainstream ABIs); `std::try_cast` can simply refuse to engage.

----

Previously on this blog:

* ["Data race when catching by non-const reference"](/blog/2018/09/16/data-race-when-catch-by-nonconst-reference/)
