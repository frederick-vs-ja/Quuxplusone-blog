---
layout: post
title: "Help wanted: Compile your codebase with P1144 and P2786 relocatability!"
date: 2024-04-10 00:01:00 +0000
tags:
  help-wanted
  llvm
  proposal
  relocatability
  wg21
excerpt: |
  At today's WG21 SG14 (Low Latency) meeting, there was a discussion of
  [P1144](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1144r10.html) trivial relocatability
  versus [P2786](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2786r4.pdf) trivial relocatability.
  It was remarked that each proposal has a corresponding Clang fork.

  * My P1144 implementation, as seen in ["Announcing Trivially Relocatable"](/blog/2018/07/18/announcing-trivially-relocatable/) (2018-07-18)
      and [on godbolt.org](https://godbolt.org/z/18hvvxjE4)

  * Corentin Jabot's P2786 implementation, as seen [TODO fill this in: [godbolt.org/z/KEY3n4P3W](https://godbolt.org/z/KEY3n4P3W)
      was shown before, but doesn't work at the moment]

  So I suggested that anyone interested in relocation could <b>really help us out</b>
  by downloading both compiler implementations and trying them out on their own
  codebases! Of course, that means you need to know how to compile them from scratch.
  Here's the answer for my P1144 implementation. I'll post the P2786 answer once
  it's relayed to me.

  > I would love to turn these instructions into a Dockerfile so that you
  > could just build a Docker container with Clang in it, and somehow build
  > your codebase with that Dockerized Clang. I've heard that VS Code actually
  > makes that "easy." So if you do it, I'd love to hear about it.
  > I'll upload the Dockerfile here and credit you.
---

At today's WG21 SG14 (Low Latency) meeting, there was a discussion of
[P1144](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1144r10.html) trivial relocatability
versus [P2786](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2786r4.pdf) trivial relocatability.
It was remarked that each proposal has a corresponding Clang fork.

* My P1144 implementation, as seen in ["Announcing Trivially Relocatable"](/blog/2018/07/18/announcing-trivially-relocatable/) (2018-07-18)
    and [on godbolt.org](https://godbolt.org/z/18hvvxjE4)

* Corentin Jabot's P2786 implementation, as seen [TODO fill this in: [godbolt.org/z/KEY3n4P3W](https://godbolt.org/z/KEY3n4P3W)
    was shown before, but doesn't work at the moment]

So I suggested that anyone interested in relocation could <b>really help us out</b>
by downloading both compiler implementations and trying them out on their own
codebases! Of course, that means you need to know how to compile them from scratch.
Here's the answer for my P1144 implementation. I'll post the P2786 answer once
it's relayed to me.

> I would love to turn these instructions into a Dockerfile so that you
> could just build a Docker container with Clang in it, and somehow build
> your codebase with that Dockerized Clang. I've heard that VS Code actually
> makes that "easy." So if you do it, I'd love to hear about it.
> I'll upload the Dockerfile here and credit you.


# Get the code and build it

```
cd ~
git clone --depth=20 --single-branch --branch=trivially-relocatable-prod \
    https://github.com/Quuxplusone/llvm-project p1144-llvm-project
cd p1144-llvm-project
git checkout origin/trivially-relocatable-prod
mkdir build
cd build
cmake -G Ninja \
    -DDEFAULT_SYSROOT="$(xcrun --show-sdk-path)" \
    -DLLVM_ENABLE_PROJECTS="clang" \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
    -DLLVM_TARGETS_TO_BUILD="AArch64;X86" \
    -DCMAKE_BUILD_TYPE=Release ../llvm
ninja clang
ninja cxx
```

# Optionally, run tests

```
ninja check-clang
ninja check-cxx
```

# Do the new features compile and link?

```
cat >hello.cpp <<EOF
 #include <stdio.h>
 #include <memory>
 #include <tuple>
 #include <type_traits>
 static_assert(std::is_trivially_relocatable<std::unique_ptr<int>>::value, "");
 struct RuleOfZero { std::unique_ptr<int> p_; };
 static_assert(std::is_trivially_relocatable<RuleOfZero>::value, "");
 struct [[trivially_relocatable]] Wrap0 {
   static_assert(not std::is_trivially_relocatable<std::tuple<int&>>::value, "");
   std::tuple<int&> t_;
 };
 static_assert(std::is_trivially_relocatable<Wrap0>::value, "");
 template<class T> void open_window(T *s, int n, int k) {
   std::uninitialized_relocate_backward(s, s+n, s+n+k);
 }
 template<class T> void close_window(T *s, int n, int k) {
   std::uninitialized_relocate(s+n, s+n+k, s);
 }
 template void open_window(Wrap0*, int, int);
 template void close_window(Wrap0*, int, int);
 int main() { puts("Success!"); }
EOF
~/p1144-llvm-project/build/bin/clang++ -std=c++11 hello.cpp -o ./a.out
./a.out
```

The above should compile fine, and print "Success!"

```
~/p1144-llvm-project/build/bin/clang++ -std=c++11 hello.cpp -S -o - | grep memmove
```

The above should find two memmoves: one in `open_window` and one in `close_window`.

# Plug it into your own code!

If your codebase compiles with Clang trunk and libc++ trunk, it will also compile fine
with the P1144-enabled Clang and libc++. There is no minimum `-std=c++XX` version required.

If your codebase requires GNU libstdc++ instead of libc++, then you'll also want to download
and build [my libstdc++ fork](https://github.com/Quuxplusone/gcc/tree/trivially-relocatable),
in order to get the full experience. I don't have good instructions for that, though,
sorry.

If your codebase requires GCC instead of Clang, then I can't (yet) help you.
I'm offering a large bounty to whomever _produces_ a GCC fork that implements P1144!
GCC hackers, email me for details.

---

If you're using my P1144-enabled libc++, you'll see some containers and algorithms immediately
get faster. Everywhere you'd expect relocation to be used, it's used. Notably, `vector::insert`
uses the equivalent of our test code's `open_window` function, and `vector::erase` uses the
equivalent of `close_window`. P1144 learned this pattern from [`folly::fbvector`](https://github.com/facebook/folly/blob/b599450/folly/FBVector.h#L1289-L1309);
you can see the fully evolved pattern in [`amc::Vector`](https://github.com/AmadeusITGroup/amc/blob/eff483f/include/amc/vectorcommon.hpp#L36-L50).

With libstdc++, you won't see much difference in speed, because I haven't implemented many
relocation optimizations in libstdc++.

Suppose your codebase has a lot of Rule-of-Five types that you'd _like_ the STL to relocate
"as if by memcpy." Then you can use P1144's new standard attribute as shown in our test code's
`struct Wrap0`. But since other compilers don't recognize that attribute (because it's _not_ standard),
you'll prefer to use the vendor-specific attribute:

    struct [[clang::trivially_relocatable]] S {
        ~~~~
    };

If your codebase has its own containers and algorithms that would benefit from relocating
trivially relocatable types "as if by memcpy," then you can express that desire using the
P1144 library API:

* `std::is_trivially_relocatable<T>`
* `std::uninitialized_relocate(first, last, dfirst)`
* `std::uninitialized_relocate_n(first, n, dfirst)`
* `std::uninitialized_relocate_backward(first, last, dlast)`
* `std::relocate_at(psource, pdest)`
* `T std::relocate(psource)`

> For this purpose, Stéphane Janel's [AMC](https://github.com/AmadeusITGroup/amc/) is a great library to study:
> he's implemented so many optimizations, very cleanly, in terms of P1144's library functions.

But since other STLs don't provide these functions (because they're _not_ standard),
you'll prefer to implement them yourself in your own namespace and just do something like:

    #ifdef __cpp_lib_trivially_relocatable
      using std::is_trivially_relocatable;
      using std::uninitialized_relocate;
      ~~~~
    #endif

to get the as-if-by-memcpy benefits on STLs that do support P1144.

> Or, instead of `using`-declarations, you could implement your namespace's functions as wrappers
> around the P1144 functions whenever they exist, as
> [HPX does](https://github.com/STEllAR-GROUP/hpx/blob/3add538/libs/core/type_support/include/hpx/type_support/uninitialized_relocation_primitives.hpp#L84-L109).

# Let us know how it went!

Finally, the most important step: Send an email to me
(at arthur.j.odwyer@gmail.com) and/or SG14 (at [sg14@lists.isocpp.org](https://lists.isocpp.org/mailman/listinfo.cgi/sg14))
and describe your experience! The important questions are all about usability and
fitness-for-purpose:

* Was switching compilers and libraries sufficiently seamless, or did something break?

* If you tried annotating your own types with `[[clang::trivially_relocatable]]`, how did that go?

* If you tried using (or wrapping) the library interface, how did that go?

* Did you see a change in performance?

---

And then, of course, repeat the process with the Clang fork implementing P2786,
and let us know how that went, too!
