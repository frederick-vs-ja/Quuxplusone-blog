---
layout: post
title: "Benchmarking Clang's `-fbuiltin-std-forward`"
date: 2022-12-24 00:01:00 +0000
tags:
  benchmarks
  compile-time-performance
  cpplang-slack
  library-design
  llvm
excerpt: |
  In 2022 we saw a lot of interest (finally!) in the costs of `std::move` and
  `std::forward`. For example, in April Richard Smith landed
  [`-fbuiltin-std-forward`](https://github.com/llvm/llvm-project/commit/72315d02c432a0fe0acae9c96c69eac8d8e1a9f6)
  in Clang; in September Vittorio Romeo lamented
  ["The sad state of debug performance in C++"](https://vittorioromeo.info/index/blog/debug_performance_cpp.html);
  and in December the MSVC team landed [`[[msvc::intrinsic]]`](https://devblogs.microsoft.com/cppblog/improving-the-state-of-debug-performance-in-c/).

  Recall that `std::forward<Arg>(arg)` should be used [only on forwarding references](/blog/2023/05/27/dont-forward-non-forwarding-references/),
  and that when you do, it's exactly equivalent to `static_cast<Arg&&>(arg)`, or equivalently `decltype(arg)(arg)`.
  But historically `std::forward` has been vastly more expensive to compile, because as far as the compiler is concerned,
  it's just a function template that needs to be instantiated, codegenned, inlined, and so on.

  Way back in March 2015 — seven and a half years ago! — Louis Dionne did a little compile-time benchmark
  and found that he could win "an improvement of [...] about 13.9%" simply by search-and-replacing all of
  Boost.Hana's `std::forward`s into `static_cast`s. [So he did that.](https://github.com/boostorg/hana/commit/540f665e5132d75bbf6eda704638622727c0c01c)

  Now, these days, Clang understands `std::forward` just like it understands `strlen`. (You can disable that
  clever behavior with `-fno-builtin-strlen`, `-fno-builtin-std-forward`.) As I understand it, this means that
  Clang will avoid generating debug info for instantiations of `std::forward`, and also inline it into the AST
  more eagerly. Basically, Clang can short-circuit some of the compile-time cost of `std::forward`. But does
  it short-circuit _enough_ of the cost to win back Louis's 13.9% improvement? Would that patch from 2015
  still pass muster today? Let's reproduce Louis's benchmark numbers and find out!
---

> UPDATE, 2022-12-26: Well, I have eggnog on my face. Turns out I used the wrong Clang binary for
> some of my benchmarks! So today I reran all of the numbers. I also took this opportunity to control
> for the cost of `#include <utility>` (which might have unfairly penalized the `std::forward` numbers),
> and to use [this Python script](/blog/code/2022-12-24-harness.py) to shuffle and interleave the benchmark iterations.
> These changes greatly reduced the observed effects!
> If you want to see the previous numbers and find out how they changed, please check
> [the git history](https://github.com/Quuxplusone/blog/compare/f0062d8df61f737a17e77aace409b16b2d8d08ff...18e4feef5434269360d8bf660a05e133946072f7).

In 2022 we saw a lot of interest (finally!) in the costs of `std::move` and
`std::forward`. For example, in April Richard Smith landed
[`-fbuiltin-std-forward`](https://github.com/llvm/llvm-project/commit/72315d02c432a0fe0acae9c96c69eac8d8e1a9f6)
in Clang; in September Vittorio Romeo lamented
["The sad state of debug performance in C++"](https://vittorioromeo.info/index/blog/debug_performance_cpp.html);
and in December the MSVC team landed [`[[msvc::intrinsic]]`](https://devblogs.microsoft.com/cppblog/improving-the-state-of-debug-performance-in-c/).

Recall that `std::forward<Arg>(arg)` should be used [only on forwarding references](/blog/2023/05/27/dont-forward-non-forwarding-references/),
and that when you do, it's exactly equivalent to `static_cast<Arg&&>(arg)`, or equivalently `decltype(arg)(arg)`.
But historically `std::forward` has been vastly more expensive to compile, because as far as the compiler is concerned,
it's just a function template that needs to be instantiated, codegenned, inlined, and so on.

Way back in March 2015 — seven and a half years ago! — Louis Dionne did a little compile-time benchmark
and found that he could win "an improvement of [...] about 13.9%" simply by search-and-replacing all of
Boost.Hana's `std::forward`s into `static_cast`s. [So he did that.](https://github.com/boostorg/hana/commit/540f665e5132d75bbf6eda704638622727c0c01c)

Now, these days, Clang understands `std::forward` just like it understands `strlen`. (You can disable that
clever behavior with `-fno-builtin-strlen`, `-fno-builtin-std-forward`.) As I understand it, this means that
Clang will avoid generating debug info for instantiations of `std::forward`, and also inline it into the AST
more eagerly. Basically, Clang can short-circuit some of the compile-time cost of `std::forward`. But does
it short-circuit _enough_ of the cost to win back Louis's 13.9% improvement? Would that patch from 2015
still pass muster today? Let's reproduce Louis's benchmark numbers and find out!

In the spirit of reproducibility, I'm going to walk through my entire benchmark-gathering process here.
If you just want to see the pretty bar graphs, you might as well [jump to the Conclusion](#conclusion).

All numbers were collected on my Macbook Pro running OS X 12.6.1, using a top-of-tree Clang and libc++
built in `RelWithDebugInfo` mode. (This Clang was actually
[my `trivially-relocatable` branch](https://github.com/Quuxplusone/llvm-project/commits/5908ed77964cc154e4493fe2b188ea40be28baca),
but that doesn't matter for our purposes.) I tried not to do anything that would grossly interfere with
the laptop's performance during the test (e.g. [hunt polyomino snakes](/blog/2022/12/08/polyomino-snakes/));
still, be aware that this experiment was poorly controlled in that respect.

Let's begin!


## Check out the old revision of Hana

First, let's build that old revision of Hana with our top-of-tree
Clang and libc++. Instructions for building Clang and libc++ (somewhat bit-rotted,
I admit) are in ["How to build LLVM from source, monorepo version"](/blog/2019/11/09/llvm-from-scratch/) (2019-11-09).

    $ git clone https://github.com/boostorg/hana/
    $ cd hana
    $ git checkout 540f665e51~

At this point you might need to make some local changes to get the old revision of Hana to build.
I had to make the following five changes:

<b>1.</b> To avoid accidentally including headers from `/usr/local/include/boost/hana`:

    CMakeLists.txt
    -find_package(Boost)
    +set(Boost_FOUND 0)

<b>2.</b> Because libc++ 16 will [change the format of `_LIBCPP_VERSION`](https://github.com/llvm/llvm-project/commit/b6ff5b470d5e4acfde59d57f18e575c0284941f4):

    include/boost/hana/config.hpp
    -#   define BOOST_HANA_CONFIG_LIBCPP BOOST_HANA_CONFIG_VERSION(              \
    -                ((_LIBCPP_VERSION) / 1000) % 10, 0, (_LIBCPP_VERSION) % 1000)
    +#   define BOOST_HANA_CONFIG_LIBCPP BOOST_HANA_CONFIG_VERSION(16, 0, 0)

<b>3.</b> To avoid a compiler warning:

    include/boost/hana/core/make.hpp
    -            static_assert((sizeof...(X), false),
    +            static_assert(((void)sizeof...(X), false),

<b>4.</b> To fix an ambiguity with the `uint` from `<sys/types.h>` on OS X:

    test/tuple.cpp
    -                prepend(uint<0>, tuple_c<unsigned int, 1>),
    +                prepend(boost::hana::uint<0>, tuple_c<unsigned int, 1>),

<b>5.</b> I also made this change up front, because we're going to need this when we
[switch to the real `std::forward`](#replace-detailstdforward-with-stdforward),
and we don't want the cost of including `<utility>` to be a confounding factor.

    $ echo '#include <utility>' >>include/boost/hana/detail/std/forward.hpp


## Build the test case

    $ mkdir build
    $ cd build
    $ cmake .. -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

This gives us an initial set of timing results:

| `detail::forward<T>`<br>`-O0`<br> including link time | user | system | real |
|---------------------------|---------|--------|---------|
| `make compile.test.tuple` | 114.115s | 6.726s | 127.441s |
| `make compile.test.tuple` | 118.420s | 6.987s | 133.369s |
| `make compile.test.tuple` | 114.480s | 6.430s | 127.353s |
| `make compile.test.tuple` | 116.171s | 6.585s | 130.244s |
{:.smaller}

## Ignore the linker

`make` is compiling _and_ linking, but we really only care about the cost of
compiling. So let's eliminate the cost of linking.
[StackOverflow](https://stackoverflow.com/questions/1867745/cmake-use-a-custom-linker)
provides this incantation:

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `detail::forward<T>`<br>`-O0` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 105.757s | 4.984s | 113.710s |
| `make compile.test.tuple` | 108.211s | 5.322s | 118.285s |
| `make compile.test.tuple` | 114.888s | 6.059s | 125.776s |
| `make compile.test.tuple` | 107.775s | 5.261s | 117.796s |
{:.smaller}

## Compare `-O0`, `-O2 -g`, and `-O3`

At this point I realize that we're getting CMake's default "Debug" build type,
which is basically `-O0` — not terribly realistic. So let's also benchmark
the build types "RelWithDebugInfo" (`-O2 -g`) and "Release" (`-O3`).

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `detail::forward<T>`<br>`-O2 -g` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 214.718s | 6.197s | 225.538s |
| `make compile.test.tuple` | 207.280s | 5.663s | 215.013s |
| `make compile.test.tuple` | 207.382s | 6.160s | 216.759s |
| `make compile.test.tuple` | 215.180s | 6.244s | 223.713s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=Release
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `detail::forward<T>`<br>`-O3` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 166.621s | 4.348s | 175.636s |
| `make compile.test.tuple` | 161.489s | 3.985s | 168.599s |
| `make compile.test.tuple` | 163.130s | 4.390s | 170.926s |
| `make compile.test.tuple` | 170.153s | 4.604s | 178.123s |
{:.smaller}


## Replace `detail::std::forward` with `std::forward`

Now, Boost.Hana actually uses a hand-rolled template named `detail::std::forward`
instead of the STL `std::forward`. That's certainly going to mess with our
numbers, if Clang doesn't realize that `detail::std::forward` behaves like
`std::forward`. Let's replace `detail::std::forward` with `std::forward`
and collect again:

    $ git stash
    $ git checkout 540f665e51~
    $ git grep -l 'std::forward' .. | xargs sed -i -e 's/::boost::hana::detail::std::forward/::std::forward/g'
    $ git grep -l 'std::forward' .. | xargs sed -i -e 's/boost::hana::detail::std::forward/::std::forward/g'
    $ git grep -l 'std::forward' .. | xargs sed -i -e 's/hana::detail::std::forward/::std::forward/g'
    $ git grep -l 'std::forward' .. | xargs sed -i -e 's/detail::std::forward/::std::forward/g'
    $ git commit -a -m 'dummy message'
    $ git stash pop

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=Debug
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `std::forward<T>`<br>`-O0` | user | system | real |
|---------------------------|---------|--------|----------|
| `make compile.test.tuple` | 103.223s | 5.352s | 115.064s |
| `make compile.test.tuple` | 100.498s | 5.166s | 110.846s |
| `make compile.test.tuple` | 100.564s | 4.951s | 109.218s |
| `make compile.test.tuple` | 101.097s | 5.087s | 113.160s |
{:.smaller}

| `std::forward<T>`<br>`-O2 -g` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 197.714s | 5.325s | 204.809s |
| `make compile.test.tuple` | 205.491s | 6.348s | 216.938s |
| `make compile.test.tuple` | 210.890s | 6.581s | 221.418s |
| `make compile.test.tuple` | 201.158s | 5.645s | 208.776s |
{:.smaller}

| `std::forward<T>`<br>`-O3` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 159.796s | 4.275s | 167.517s |
| `make compile.test.tuple` | 156.718s | 4.398s | 164.323s |
| `make compile.test.tuple` | 160.336s | 4.120s | 169.296s |
| `make compile.test.tuple` | 166.621s | 4.760s | 175.783s |
{:.smaller}


## Compare `-fno-builtin-std-forward`

My understanding is that all of Clang's special handling for `std::forward`
can be toggled on and off via `-fno-builtin-std-forward`
(see [the relevant commit](https://github.com/llvm/llvm-project/commit/72315d02c432a0fe0acae9c96c69eac8d8e1a9f6)).
So we should discover if `-fno-builtin-std-forward` actually does slow down the
compile.

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_CXX_FLAGS=-fno-builtin-std-forward \
          -DCMAKE_BUILD_TYPE=Debug
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `std::forward<T> -O0`<br>`-fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 108.377s | 5.352s | 120.684s |
| `make compile.test.tuple` | 112.791s | 5.913s | 125.567s |
| `make compile.test.tuple` | 110.949s | 5.461s | 121.821s |
| `make compile.test.tuple` | 108.780s | 5.253s | 118.924s |
{:.smaller}

| `std::forward<T> -O2 -g`<br>`-fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 222.592s | 7.022s | 233.819s |
| `make compile.test.tuple` | 220.630s | 6.608s | 230.786s |
| `make compile.test.tuple` | 216.572s | 6.156s | 226.708s |
| `make compile.test.tuple` | 212.081s | 5.916s | 221.148s |
{:.smaller}

| `std::forward<T> -O3`<br>`-fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 160.622s | 3.822s | 167.232s |
| `make compile.test.tuple` | 164.214s | 4.055s | 171.536s |
| `make compile.test.tuple` | 171.738s | 4.612s | 180.935s |
| `make compile.test.tuple` | 163.612s | 4.103s | 171.175s |
{:.smaller}

We should also collect these numbers for the original `detail::std::forward`.

| `detail::forward<T> -O0`<br>`-fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 109.730s | 5.318s | 120.951s |
| `make compile.test.tuple` | 108.594s | 5.197s | 120.456s |
| `make compile.test.tuple` | 106.325s | 5.006s | 116.721s |
| `make compile.test.tuple` | 106.106s | 4.907s | 116.015s |
{:.smaller}

| `detail::forward<T> -O2 -g`<br>`-fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 213.302s | 6.133s | 222.003s |
| `make compile.test.tuple` | 213.664s | 6.035s | 221.804s |
| `make compile.test.tuple` | 221.149s | 6.862s | 231.551s |
| `make compile.test.tuple` | 209.036s | 5.749s | 217.968s |
{:.smaller}

| `detail::forward<T> -O3`<br>`-fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 161.180s | 4.017s | 168.324s |
| `make compile.test.tuple` | 170.037s | 4.576s | 178.451s |
| `make compile.test.tuple` | 167.476s | 4.481s | 179.675s |
| `make compile.test.tuple` | 164.460s | 4.157s | 169.649s |
{:.smaller}


## Switch to `static_cast<T&&>`

Now the moment we've been waiting for!
Apply the commit that switched Hana from `detail::std::forward` to `static_cast<T&&>`:

    $ git stash
    $ git checkout 540f665e51
    $ git stash pop

and run all the same benchmarks again:

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `static_cast<T&&>`<br>`-O0` | user | system | real |
|---------------------------|---------|--------|---------|
| `make compile.test.tuple` | 91.421s | 4.558s | 102.832s |
| `make compile.test.tuple` | 95.103s | 4.918s | 106.348s |
| `make compile.test.tuple` | 95.386s | 4.780s | 105.589s |
| `make compile.test.tuple` | 98.711s | 5.516s | 111.344s |
{:.smaller}

| `static_cast<T&&>`<br>`-O2 -g` | user | system | real |
|---------------------------|----------|--------|---------|
| `make compile.test.tuple` | 202.589s | 5.949s | 211.770s |
| `make compile.test.tuple` | 194.883s | 5.456s | 202.536s |
| `make compile.test.tuple` | 196.569s | 5.693s | 207.602s |
| `make compile.test.tuple` | 193.946s | 5.444s | 201.535s |
{:.smaller}

| `static_cast<T&&>`<br>`-O3` | user | system | real |
|---------------------------|----------|--------|---------|
| `make compile.test.tuple` | 153.371s | 3.884s | 159.933s |
| `make compile.test.tuple` | 149.820s | 3.681s | 156.982s |
| `make compile.test.tuple` | 153.347s | 4.038s | 161.922s |
| `make compile.test.tuple` | 151.943s | 3.947s | 159.044s |
{:.smaller}

| `static_cast<T&&> -O0`<br>`-fno-builtin-std-forward` | user | system | real |
|---------------------------|---------|--------|----------|
| `make compile.test.tuple` | 96.991s | 5.295s | 109.769s |
| `make compile.test.tuple` | 93.719s | 4.668s | 105.419s |
| `make compile.test.tuple` | 94.057s | 4.684s | 99.843s |
| `make compile.test.tuple` | 93.761s | 4.699s | 103.461s |
{:.smaller}

| `static_cast<T&&> -O2 -g`<br>`-fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 191.906s | 5.193s | 199.158s |
| `make compile.test.tuple` | 196.225s | 5.444s | 204.091s |
| `make compile.test.tuple` | 194.927s | 5.341s | 202.128s |
| `make compile.test.tuple` | 201.406s | 5.967s | 210.618s |
{:.smaller}

| `static_cast<T&&> -O3`<br>`-fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 153.070s | 3.940s | 160.831s |
| `make compile.test.tuple` | 155.854s | 3.989s | 164.267s |
| `make compile.test.tuple` | 150.787s | 3.783s | 158.836s |
| `make compile.test.tuple` | 149.737s | 3.739s | 157.007s |
{:.smaller}


## Conclusion

All of the numbers above, collated into a single bar graph:

![](/blog/images/2022-12-24-builtin-std-forward.svg)

The `-fno-builtin` numbers are kind of silly, because ordinary programmers won't
be toggling that option and because it only hurts anyway. So here's the same graph
without the `-fno-builtin` bars:

![Seasonal coloration](/blog/images/2022-12-24-builtin-std-forward-simplified.svg)

On top-of-tree Clang, Boost.Hana's replacement of `std::forward<T>` with `static_cast<T&&>`
produces a compile-speed improvement somewhere between 3 and 6 percent on Louis's benchmark.
Replacing the opaque `detail::std::forward` with `static_cast` gives 7 to 10 percent.
Both are significantly lessened from the 13.9% speedup Louis saw on his own machine back in 2015.

Note that switching from `detail::std::forward` (unrecognized by Clang) to
the standard `std::forward` helps quite a bit on this benchmark, indicating that
`-fbuiltin-std-forward` is doing its job. The opt-out flag `-fno-builtin-std-forward` does,
as expected, return things to their previous (worse) state.

Practically speaking, would you see any compile-time speedup if you made a similar change to your own codebase?
Unlikely, unless your codebase looks a lot like this one.
Hana sees a speedup because it uses a huge amount of perfect forwarding, and because
this specific benchmark is a stress test focused on `std::tuple`. Contrariwise, the typical industry
codebase ought to spend most of its time compiling non-templates, and use `std::forward` only rarely.
But if you're a library writer, it seems that "for compile-time performance, avoid instantiating
`std::forward`" is still plausible advice: _much less_ applicable today on Clang than it was seven years
ago (or even one year ago), but a small effect is still noticeable.

I'd be interested to hear the results of this benchmark on a recent GCC or MSVC.

---

See also:

* ["Prefer core-language features over library facilities"](/blog/2022/10/16/prefer-core-over-library/) (2022-10-16)
