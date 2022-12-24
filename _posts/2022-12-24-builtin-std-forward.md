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
I had to make the following four changes.

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

## Build the test case

    $ mkdir build
    $ cd build
    $ cmake .. -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

This gives us an initial set of timing results:

| `detail::forward<T>`<br>`-O0`<br> including link time | user | system | real |
|---------------------------|---------|--------|---------|
| `make compile.test.tuple` | 84.424s | 4.458s | 89.555s |
| `make compile.test.tuple` | 82.242s | 4.268s | 86.967s |
| `make compile.test.tuple` | 82.547s | 4.250s | 87.280s |
| `make compile.test.tuple` | 83.257s | 4.252s | 87.887s |
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
|---------------------------|---------|--------|---------|
| `make compile.test.tuple` | 80.572s | 3.554s | 86.178s |
| `make compile.test.tuple` | 79.737s | 3.483s | 84.713s |
| `make compile.test.tuple` | 80.750s | 3.569s | 85.307s |
| `make compile.test.tuple` | 79.939s | 3.417s | 84.149s |
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

| `detail::forward<T>`<br>`-O2 -g`, excluding link time | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 221.035s | 6.913s | 229.956s |
| `make compile.test.tuple` | 217.392s | 6.862s | 225.573s |
| `make compile.test.tuple` | 221.768s | 7.225s | 232.986s |
| `make compile.test.tuple` | 219.901s | 6.967s | 229.589s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=Release
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `detail::forward<T>`<br>`-O3` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 164.884s | 4.435s | 170.144s |
| `make compile.test.tuple` | 163.737s | 4.409s | 169.438s |
| `make compile.test.tuple` | 162.173s | 4.274s | 167.295s |
| `make compile.test.tuple` | 162.880s | 4.305s | 167.894s |
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
    $ echo '#include <utility>' >>include/boost/hana/detail/std/forward.hpp
    $ git commit -a -m 'dummy message'
    $ git stash pop

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=Debug
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `std::forward<T>`<br>`-O0` | user | system | real |
|---------------------------|---------|--------|----------|
| `make compile.test.tuple` | 96.989s | 4.205s | 102.347s |
| `make compile.test.tuple` | 93.747s | 4.061s |  98.442s |
| `make compile.test.tuple` | 94.190s | 4.078s |  98.936s |
| `make compile.test.tuple` | 93.716s | 3.982s |  98.372s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `std::forward<T>`<br>`-O2 -g` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 218.415s | 6.895s | 228.219s |
| `make compile.test.tuple` | 210.701s | 6.376s | 219.064s |
| `make compile.test.tuple` | 211.242s | 6.379s | 219.168s |
| `make compile.test.tuple` | 216.921s | 6.773s | 225.744s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=Release
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `std::forward<T>`<br>`-O3` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 162.218s | 4.360s | 167.295s |
| `make compile.test.tuple` | 159.797s | 4.244s | 164.605s |
| `make compile.test.tuple` | 159.719s | 4.230s | 164.631s |
| `make compile.test.tuple` | 159.551s | 4.234s | 164.806s |
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
|---------------------------|---------|--------|----------|
| `make compile.test.tuple` | 99.215s | 4.666s | 105.764s |
| `make compile.test.tuple` | 96.094s | 4.324s | 101.277s |
| `make compile.test.tuple` | 99.047s | 4.566s | 105.448s |
| `make compile.test.tuple` | 99.955s | 4.684s | 105.485s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_CXX_FLAGS=-fno-builtin-std-forward \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `std::forward<T>`<br>`-O2 -g -fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 210.530s | 6.547s | 218.800s |
| `make compile.test.tuple` | 210.457s | 6.506s | 218.027s |
| `make compile.test.tuple` | 212.517s | 6.598s | 220.619s |
| `make compile.test.tuple` | 215.020s | 6.686s | 223.429s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_CXX_FLAGS=-fno-builtin-std-forward \
          -DCMAKE_BUILD_TYPE=Release
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `std::forward<T>`<br>`-O3 -fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 174.529s | 5.181s | 182.072s |
| `make compile.test.tuple` | 165.689s | 4.640s | 172.359s |
| `make compile.test.tuple` | 163.069s | 4.419s | 168.253s |
| `make compile.test.tuple` | 159.797s | 4.215s | 164.591s |
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
| `make compile.test.tuple` | 68.520s | 2.996s | 72.412s |
| `make compile.test.tuple` | 66.733s | 3.008s | 70.380s |
| `make compile.test.tuple` | 67.924s | 3.042s | 71.685s |
| `make compile.test.tuple` | 68.961s | 3.158s | 72.901s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `static_cast<T&&>`<br>`-O2 -g` | user | system | real |
|---------------------------|----------|--------|---------|
| `make compile.test.tuple` | 199.933s | 6.057s | 207.683s |
| `make compile.test.tuple` | 194.558s | 5.559s | 200.970s |
| `make compile.test.tuple` | 198.808s | 6.022s | 205.881s |
| `make compile.test.tuple` | 195.495s | 5.889s | 202.192s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_BUILD_TYPE=Release
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `static_cast<T&&>`<br>`-O3` | user | system | real |
|---------------------------|----------|--------|---------|
| `make compile.test.tuple` | 153.146s | 4.150s | 158.101s |
| `make compile.test.tuple` | 150.003s | 3.972s | 155.025s |
| `make compile.test.tuple` | 152.015s | 4.183s | 156.924s |
| `make compile.test.tuple` | 151.421s | 4.046s | 156.146s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_CXX_FLAGS=-fno-builtin-std-forward \
          -DCMAKE_BUILD_TYPE=Debug
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `static_cast<T&&>`<br>`-O0 -fno-builtin-std-forward` | user | system | real |
|---------------------------|---------|--------|----------|
| `make compile.test.tuple` | 89.814s | 4.363s | 96.116s |
| `make compile.test.tuple` | 85.631s | 4.000s | 90.547s |
| `make compile.test.tuple` | 87.802s | 4.230s | 94.220s |
| `make compile.test.tuple` | 88.962s | 4.373s | 96.161s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_CXX_FLAGS=-fno-builtin-std-forward \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `static_cast<T&&>`<br>`-O2 -g -fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 220.881s | 8.070s | 234.485s |
| `make compile.test.tuple` | 201.314s | 6.152s | 208.810s |
| `make compile.test.tuple` | 202.640s | 6.281s | 210.371s |
| `make compile.test.tuple` | 198.024s | 5.817s | 204.780s |
{:.smaller}

    $ cd .. ; rm -rf build ; mkdir build ; cd build
    $ cmake .. -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true \
          -DCMAKE_CXX_COMPILER=$HOME/llvm-project/build/bin/clang++ \
          -DCMAKE_CXX_FLAGS=-fno-builtin-std-forward \
          -DCMAKE_BUILD_TYPE=Release
    $ for i in 1 2 3 4; do make clean; time make -j1 compile.test.tuple; done

| `static_cast<T&&>`<br>`-O3 -fno-builtin-std-forward` | user | system | real |
|---------------------------|----------|--------|----------|
| `make compile.test.tuple` | 156.198s | 4.289s | 161.519s |
| `make compile.test.tuple` | 154.399s | 4.248s | 159.645s |
| `make compile.test.tuple` | 157.304s | 4.499s | 164.054s |
| `make compile.test.tuple` | 153.200s | 4.126s | 158.171s |
{:.smaller}


## Conclusion

All of the numbers above, collated into a single bar graph:

![](/blog/images/2022-12-24-builtin-std-forward.svg)

The `-fno-builtin` numbers are kind of silly, because ordinary programmers won't
be toggling that option and because it only hurts anyway. So here's the same graph
without the `-fno-builtin` bars:

![Seasonal coloration](/blog/images/2022-12-24-builtin-std-forward-simplified.svg)

Even on top-of-tree Clang, Boost.Hana's replacement of `std::forward<T>` with `static_cast<T&&>`
still produces a compile-speed improvement somewhere between 5 and 28 percent on Louis's benchmark.
That's massive — and not significantly changed from the 13.9% speedup Louis saw on his own machine
back in 2015.

We do observe that it's a bad idea to opt out of the optimization by passing
`-fno-builtin-std-forward`. (Of course; but it's nice that the data show it.)
Notice that `-fno-builtin-std-forward` has an effect even for the `static_cast<T&&>` bars;
that's because libc++ uses `std::forward` itself internally.

Practically speaking, would you see a similar speedup if you made a similar change to your own codebase?
Probably not, unless your codebase looks a lot like Hana.
Hana sees a huge speedup because it uses a huge amount of perfect forwarding, and because
this specific benchmark is a stress test focused on `std::tuple`. Contrariwise, the typical industry
codebase ought to spend most of its time compiling non-templates, and use `std::forward` only rarely.
But if you're a library writer, it seems that "for compile-time performance, avoid instantiating
`std::forward`" is no less applicable today than it was seven years ago.

---

See also:

* ["Prefer core-language features over library facilities"](/blog/2022/10/16/prefer-core-over-library/) (2022-10-16)
