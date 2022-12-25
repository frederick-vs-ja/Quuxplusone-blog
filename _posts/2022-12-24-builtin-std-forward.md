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
  in Clang, and in September Vittorio Romeo lamented
  ["The sad state of debug performance in C++"](https://vittorioromeo.info/index/blog/debug_performance_cpp.html).

  Recall that `std::forward<Arg>(arg)` should be used [only on forwarding references](/blog/2022/02/02/look-what-they-need/),
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

> UPDATE, 2022-12-25: Well, I have eggnog on my face. Turns out I used the wrong Clang binary for
> some of my benchmarks! So today I reran all of the numbers. I also took this opportunity to control
> for the cost of `#include <utility>`, which might have unfairly penalized the `std::forward` numbers.
> These changes practically eliminated the observed effects at `-O2` and `-O3`!
> If you want to see the previous numbers and find out how they changed, please check
> [the git history]().

In 2022 we saw a lot of interest (finally!) in the costs of `std::move` and
`std::forward`. For example, in April Richard Smith landed
[`-fbuiltin-std-forward`](https://github.com/llvm/llvm-project/commit/72315d02c432a0fe0acae9c96c69eac8d8e1a9f6)
in Clang, and in September Vittorio Romeo lamented
["The sad state of debug performance in C++"](https://vittorioromeo.info/index/blog/debug_performance_cpp.html).

Recall that `std::forward<Arg>(arg)` should be used [only on forwarding references](/blog/2022/02/02/look-what-they-need/),
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
I had to make the following five changes.

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
| `make compile.test.tuple` | 82.344s | 4.172s | 87.330s |
| `make compile.test.tuple` | 81.244s | 4.101s | 85.581s |
| `make compile.test.tuple` | 84.793s | 4.477s | 90.335s |
| `make compile.test.tuple` | 82.913s | 4.254s | 87.494s |
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
| `make compile.test.tuple` | 102.424s | 4.774s | 108.066s |
| `make compile.test.tuple` |  98.797s | 4.509s | 104.081s |
| `make compile.test.tuple` |  98.696s | 4.433s | 103.805s |
| `make compile.test.tuple` |  98.150s | 4.439s | 103.223s |
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
| `make compile.test.tuple` | 201.835s | 5.319s | 208.002s |
| `make compile.test.tuple` | 198.748s | 5.124s | 204.587s |
| `make compile.test.tuple` | 197.826s | 4.977s | 203.408s |
| `make compile.test.tuple` | 198.257s | 4.976s | 203.810s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=Release
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `detail::forward<T>`<br>`-O3` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 151.065s | 3.298s | 154.963s |
| `make compile.test.tuple` | 150.266s | 3.314s | 154.064s |
| `make compile.test.tuple` | 150.521s | 3.310s | 154.798s |
| `make compile.test.tuple` | 150.390s | 3.317s | 154.210s |
{:.smaller}


## Replace `detail::std::forward` with `std::forward`

Now, Boost.Hana actually uses a hand-rolled template named `detail::std::forward`
instead of the actual STL `std::forward`. That's certainly going to mess with our
numbers, if Clang doesn't realize that `detail::std::forward` is a synonym for
`std::forward`. (In the end, the data will indicate that `std::forward` and `detail::std::forward`
perform pretty much the same.) Let's replace `detail::std::forward` with `std::forward`
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
| `make compile.test.tuple` | 88.553s | 3.938s |  93.110s |
| `make compile.test.tuple` | 89.222s | 4.000s |  93.864s |
| `make compile.test.tuple` | 95.202s | 4.622s | 100.686s |
| `make compile.test.tuple` | 92.416s | 4.320s |  97.466s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `std::forward<T>`<br>`-O2 -g` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 194.538s | 5.179s | 200.581s |
| `make compile.test.tuple` | 190.392s | 5.040s | 196.082s |
| `make compile.test.tuple` | 190.625s | 5.074s | 196.337s |
| `make compile.test.tuple` | 189.850s | 5.045s | 195.536s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=Release
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `std::forward<T>`<br>`-O3` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 151.877s | 3.873s | 156.633s |
| `make compile.test.tuple` | 145.784s | 3.459s | 149.777s |
| `make compile.test.tuple` | 146.058s | 3.508s | 150.233s |
| `make compile.test.tuple` | 146.297s | 3.491s | 150.563s |
{:.smaller}


## Compare `-fno-builtin-std-forward`

My understanding is that all of Clang's special handling for `std::forward`
can be toggled on and off via `-fno-builtin-std-forward`
(see [the relevant April 2022 commit](https://github.com/llvm/llvm-project/commit/72315d02c432a0fe0acae9c96c69eac8d8e1a9f6)).
So we should discover if `-fno-builtin-std-forward` actually does slow down the
compile.

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_CXX_FLAGS=-fno-builtin-std-forward \
          -DCMAKE_BUILD_TYPE=Debug
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `std::forward<T>`<br>`-O0 -fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 103.059s | 4.628s | 108.567s |
| `make compile.test.tuple` |  98.347s | 4.375s | 103.419s |
| `make compile.test.tuple` |  98.467s | 4.429s | 103.717s |
| `make compile.test.tuple` |  97.754s | 4.314s | 102.745s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_CXX_FLAGS=-fno-builtin-std-forward \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `std::forward<T>`<br>`-O2 -g -fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 200.580s | 5.163s | 206.508s |
| `make compile.test.tuple` | 209.466s | 5.804s | 216.405s |
| `make compile.test.tuple` | 199.703s | 5.209s | 205.619s |
| `make compile.test.tuple` | 198.682s | 5.161s | 204.487s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_CXX_FLAGS=-fno-builtin-std-forward \
          -DCMAKE_BUILD_TYPE=Release
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `std::forward<T>`<br>`-O3 -fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 151.541s | 3.371s | 155.614s |
| `make compile.test.tuple` | 151.737s | 3.412s | 155.731s |
| `make compile.test.tuple` | 151.931s | 3.423s | 155.918s |
| `make compile.test.tuple` | 151.961s | 3.453s | 155.928s |
{:.smaller}


## Switch to `static_cast<T&&>`!

Now for the moment we've been waiting for!
Apply the commit that switched Hana from `std::forward` to `static_cast<T&&>`:

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
| `make compile.test.tuple` | 86.773s | 3.995s | 91.447s |
| `make compile.test.tuple` | 84.633s | 3.914s | 89.144s |
| `make compile.test.tuple` | 82.947s | 3.756s | 87.271s |
| `make compile.test.tuple` | 83.218s | 3.770s | 87.584s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `static_cast<T&&>`<br>`-O2 -g` | user | system | real |
|---------------------------|----------|--------|---------|
| `make compile.test.tuple` | 183.965s | 4.689s | 189.329s |
| `make compile.test.tuple` | 192.803s | 5.414s | 199.231s |
| `make compile.test.tuple` | 190.215s | 5.222s | 196.282s |
| `make compile.test.tuple` | 187.584s | 4.950s | 193.301s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=Release
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `static_cast<T&&>`<br>`-O3` | user | system | real |
|---------------------------|----------|--------|---------|
| `make compile.test.tuple` | 142.249s | 3.322s | 146.925s |
| `make compile.test.tuple` | 140.922s | 3.265s | 144.772s |
| `make compile.test.tuple` | 140.088s | 3.292s | 144.578s |
| `make compile.test.tuple` | 139.409s | 3.216s | 143.881s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_CXX_FLAGS=-fno-builtin-std-forward \
          -DCMAKE_BUILD_TYPE=Debug
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `static_cast<T&&>`<br>`-O0 -fno-builtin-std-forward` | user | system | real |
|---------------------------|---------|--------|----------|
| `make compile.test.tuple` | 83.990s | 3.802s | 88.519s |
| `make compile.test.tuple` | 83.426s | 3.784s | 87.831s |
| `make compile.test.tuple` | 84.804s | 3.961s | 89.741s |
| `make compile.test.tuple` | 88.622s | 4.257s | 93.676s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_CXX_FLAGS=-fno-builtin-std-forward \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `static_cast<T&&>`<br>`-O2 -g -fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 186.031s | 4.909s | 191.701s |
| `make compile.test.tuple` | 185.311s | 4.872s | 190.838s |
| `make compile.test.tuple` | 187.472s | 5.009s | 193.205s |
| `make compile.test.tuple` | 185.348s | 4.897s | 190.896s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_CXX_FLAGS=-fno-builtin-std-forward \
          -DCMAKE_BUILD_TYPE=Release
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `static_cast<T&&>`<br>`-O3 -fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 143.341s | 3.384s | 147.518s |
| `make compile.test.tuple` | 138.456s | 3.063s | 142.041s |
| `make compile.test.tuple` | 144.837s | 3.477s | 149.169s |
| `make compile.test.tuple` | 140.009s | 3.230s | 143.784s |
{:.smaller}


## Conclusion

All of the numbers above, collated into a single bar graph:

![](/blog/images/2022-12-24-builtin-std-forward.svg)

The `-fno-builtin` numbers are kind of silly, because ordinary programmers won't
be toggling that option and because it only hurts anyway. So here's the same graph
without the `-fno-builtin` bars:

![Seasonal coloration](/blog/images/2022-12-24-builtin-std-forward-simplified.svg)

On top-of-tree Clang, Boost.Hana's replacement of `std::forward<T>` with `static_cast<T&&>`
produces a compile-speed improvement somewhere between 1 and 8 percent on Louis's benchmark.
That's significantly changed from the 13.9% speedup Louis saw on his own machine back in 2015.
I would even say that the `-O2` and `-O3` speedups are in the noise,
although 8% at `-O0` is still concerning.

Note that switching from `detail::std::forward` (unrecognized by Clang) to
the standard `std::forward` helps quite a bit on this benchmark, indicating that
`-fbuiltin-std-forward` is doing its job. The opt-out flag `-fno-builtin-std-forward` does,
as expected, return things to their previous (worse) state.

Practically speaking, would you see an 8% speedup at `-O0` if you made a similar change to your own codebase?
Unlikely, unless your codebase looks a lot like Hana.
Hana sees a speedup because it uses a huge amount of perfect forwarding, and because
this specific benchmark is a stress test focused on `std::tuple`. Contrariwise, the typical industry
codebase ought to spend most of its time compiling non-templates, and use `std::forward` only rarely.
But if you're a library writer, it seems that "for compile-time performance, avoid instantiating
`std::forward`" is still plausible advice: _less_ applicable today on Clang than it was seven years
ago (or even one year ago), but an effect is still noticeable at `-O0`.

---

See also:

* ["Prefer core-language features over library facilities"](/blog/2022/10/16/prefer-core-over-library/) (2022-10-16)
