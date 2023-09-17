---
layout: post
title: 'An example where `inline constexpr` makes a difference'
date: 2022-07-08 00:01:00 +0000
tags:
  constexpr
  cpplang-slack
  implementation-divergence
  llvm
  standard-library-trivia
excerpt: |
  The C++11 and C++14 standard libraries defined many constexpr global variables like this:

      constexpr piecewise_construct_t piecewise_construct = piecewise_construct_t();
      constexpr allocator_arg_t allocator_arg = allocator_arg_t();
      constexpr error_type error_ctype = /*unspecified*/;
      constexpr defer_lock_t defer_lock{};
      constexpr in_place_t in_place{};
      constexpr nullopt_t nullopt(/*unspecified*/{});

  In C++17, all of these constexpr variables were respecified as `inline constexpr`
  variables. The `inline` keyword here means the same thing as it does on an `inline`
  function: "This entity might be defined in multiple [TUs](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#tu);
  all those definitions are identical; merge them into one definition at link time."
  If you look at the generated code for one of these variables in C++14, you'll
  see something like this ([Godbolt](https://godbolt.org/z/Psaxq8hfj)):
---

The C++11 and C++14 standard libraries defined many constexpr global variables like this:

    constexpr piecewise_construct_t piecewise_construct = piecewise_construct_t();
    constexpr allocator_arg_t allocator_arg = allocator_arg_t();
    constexpr error_type error_ctype = /*unspecified*/;
    constexpr defer_lock_t defer_lock{};
    constexpr in_place_t in_place{};
    constexpr nullopt_t nullopt(/*unspecified*/{});

In C++17, all of these constexpr variables were respecified as `inline constexpr`
variables. The `inline` keyword here means the same thing as it does on an `inline`
function: "This entity might be defined in multiple [TUs](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#tu);
all those definitions are identical; merge them into one definition at link time."
If you look at the generated code for one of these variables in C++14, you'll
see something like this ([Godbolt](https://godbolt.org/z/Psaxq8hfj)):

      // constexpr in_place_t in_place{};
      .section .rodata
      .type _ZStL8in_place, @object
      .size _ZStL8in_place, 1
    _ZStL8in_place:
      .zero 1

Whereas in C++17, you'll see this instead:

      // inline constexpr in_place_t in_place{};
      .weak _ZSt8in_place
      .section .rodata._ZSt8in_place,"aG",@progbits,_ZSt8in_place,comdat
      .type _ZSt8in_place, @gnu_unique_object
      .size _ZSt8in_place, 1
    _ZSt8in_place:
      .zero 1

The critical word in the latter snippet is [`comdat`](https://maskray.me/blog/2021-07-25-comdat-and-section-group);
it means "Hey linker! Instead of concatenating the text of all `.rodata._ZSt8in_place` sections together,
you should deduplicate them, so that only one such section is included in the final executable!"
There's another minor difference in the name-mangling of `std::in_place` itself: as an
`inline constexpr` variable it gets mangled into `_ZSt8in_place`, but as a non-`inline`
(and therefore `static`) variable it gets mangled into `_ZStL8in_place` with an `L`.
Clang's name-mangling code [has this to say](https://github.com/llvm/llvm-project/blob/2e603c6/clang/lib/AST/ItaniumMangle.cpp#L1458-L1472)
about the `L`:

> GCC distinguishes between internal and external linkage symbols in
> its mangling, to support cases like this that were valid C++ prior
> to [CWG 426](https://cplusplus.github.io/CWG/issues/426.html):
>
>      void test() { extern void foo(); }
>      static void foo();

On [the cpplang Slack](https://cppalliance.org/slack/), Ed Catmur showed an example
of how this difference can be observed. This is a contrived example, for sure, but
it does concretely demonstrate the difference between mere `constexpr` (internal linkage,
one entity per TU) and `inline constexpr` (external linkage, one entity for the entire
program).

    // f.hpp
    INLINE constexpr int x = 3;
    inline const void *f() { return &x; }
    using FT = const void*();
    FT *alpha();

    // alpha.cpp
    #include "f.hpp"
    FT *alpha() { return f; }

    // main.cpp
    #include <cassert>
    #include <cstdio>
    #include "f.hpp"
    int main() {
        assert(alpha() == f);      // OK
        assert(alpha()() == f());  // Fail!
        puts("Success!");
    }

    $ g++ -std=c++17 -O2 main.cpp alpha.cpp -DINLINE=inline ; ./a.out
    Success!
    $ g++ -std=c++17 -O2 alpha.cpp main.cpp -DINLINE=inline ; ./a.out
    Success!
    $ g++ -std=c++17 -O2 main.cpp alpha.cpp -DINLINE= ; ./a.out
    Success!
    $ g++ -std=c++17 -O2 alpha.cpp main.cpp -DINLINE= ; ./a.out
    a.out: main.cpp:5: int main(): Assertion `alpha()() == f()' failed.
    Aborted

The difference between the last two command lines is whether the linker
sees `alpha.o` or `main.o` first, and therefore whether it chooses to
keep the definition of `inline const void *f()` from `alpha.cpp` or `main.cpp`.
If it keeps the one from `alpha.cpp`, then the result of the expression `alpha()()`
will be the address of `x`-from-`alpha.cpp`. But, thanks to the inliner,
the assertion in `main` will be comparing that address against the
address of `x`-from-`main.cpp`. When `x` is marked `inline`, there's only
one entity `x` in the whole program, so the two `x`s are the same and
the assertion succeeds. But when `x` is a plain old `constexpr` variable,
there are two different (internal-linkage) `x`s with two different addresses,
and so the assertion fails.

You can reproduce this behavior with libstdc++, by swapping the variable `x`
for a C++14 standard library variable like `std::piecewise_construct`. The
assertion in `main` will pass when compiled with `-std=c++17` and fail when
compiled with `-std=c++14`. This is because libstdc++ makes these variables
conditionally `inline` depending on the language mode
([source](https://github.com/gcc-mirror/gcc/blob/e614925/libstdc%2B%2B-v3/include/bits/stl_pair.h#L82-L84)):

    #ifndef _GLIBCXX17_INLINE
    # if __cplusplus > 201402L
    #  define _GLIBCXX17_INLINE inline
    # else
    #  define _GLIBCXX17_INLINE
    # endif
    #endif

    _GLIBCXX17_INLINE constexpr
      piecewise_construct_t piecewise_construct =
        piecewise_construct_t();

LLVM/Clang's libc++, on the other hand, doesn't conditionalize their code
based on the language mode ([source](https://github.com/llvm/llvm-project/blob/368faac/libcxx/include/__utility/piecewise_construct.h)):

    /* inline */ constexpr
      piecewise_construct_t piecewise_construct =
        piecewise_construct_t();

I speculate that this was done in order to reduce the
confusion that could result if a program was compiled partly as C++14
and partly as C++17. It's bad enough that a program's behavior can
change (in this contrived scenario) depending on whether it's compiled
as C++14 or C++17; imagine the _additional_ confusion if some parts of the program
believed there was only one `std::piecewise_construct` and other parts
believed there were several.

> Analogously, a polymorphic class can use multiple inheritance to hold
> many base-class subobjects of the same type `Animal`; or
> it can use multiple virtual inheritance to hold a single
> virtual base-class subobject of type `Animal`.
> But imagine the confusion if
> a polymorphic class inherits both virtually and non-virtually
> from the same type!
> In ["`dynamic_cast` From Scratch"](https://youtu.be/QzJL-8WbpuU?t=1507s) (CppCon 2017),
> I used the names `CatDog` and `Nemo`, respectively,
> for the two reasonable scenarios, and `SiameseCat`-with-`Flea`
> for the confusing scenario. The MISRA-C++ coding standard
> [explicitly bans](https://rules.sonarsource.com/cpp/tag/based-on-misra/RSPEC-1013)
> the confusing scenario (by MISRA rule 10-1-3).

libc++ aggressively drops support for compilers older than about two years.
I expect that at some point all supported compilers will permit `inline constexpr`
as an extension even in C++11 mode, and then libc++ will be free to add `inline`
to all its global variables in one fell swoop.
