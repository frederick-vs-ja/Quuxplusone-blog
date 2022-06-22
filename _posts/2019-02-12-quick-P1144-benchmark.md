---
layout: post
title: "Quick and unscientific `trivially_relocatable` benchmark"
date: 2019-02-12 00:01:00 +0000
tags:
  benchmarks
  kona-2019
  relocatability
---

P1144 `[[trivially_relocatable]]` is one of those optimizations (like move semantics
and copy elision) where you can make a benchmark show any improvement you want,
because the whole point of the optimization is to eliminate arbitrarily complicated
user code. So for example at C++Now 2018 I showed a 3x speedup on
`vector<unique_ptr<int>>::reserve`; but I could just as well have shown an <i>N</i>x speedup
on `vector<deque<T>>::reserve`, where the value of <i>N</i> depends purely on your choice of `T`
— could be 2x, could be 20x, could be 200x.
(libstdc++'s `deque<T>` is the pathological case: it's trivially relocatable but
not nothrow-move-constructible. That's the key to understanding
[Marc Glisse's recent libstdc++ patches](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87106).)

So when we talk about "benchmarks for `[[trivially_relocatable]]`," what we really mean is
"how much does it improve the performance of _real software_," not contrived benchmarks.

For a long time, I've had "build LLVM/Clang both ways and compare its performance" on my
to-do list. But even that wouldn't be much of a fair comparison, because LLVM/Clang already
has a lot of smart people who've spent years making sure that LLVM/Clang is fast.
They generally don't use `std::vector`; they don't use `std::string`; they don't even use
`std::sort`. So I actually wouldn't expect Clang to get faster simply by using a P1144-enabled
compiler and standard library. (Remember, the compiler changes don't do anything by themselves;
you need a library that can take advantage of the new information P1144 provides.)

But last night I did serendipitously run some completely unscientific benchmarks on
[my Homeworlds AI](https://github.com/Quuxplusone/Homeworlds). Its bottleneck is in the
move-generation code, where it keeps a giant `std::unordered_set<std::string>` representing
(the serialized forms of) all the game states it's explored so far.

We can compare the performance of some game-tree searches using `unordered_set`, using
`set`, and using [P1222 `flat_set`](https://github.com/WG21-SG14/SG14/pull/148) since I
just implemented that.

> Notice that "many dynamic insertions, no searches" is the _absolute stupidest case_ for `flat_set`.
> You'd never use a `flat_set` like this in the real world.

I ran my existing game-tree search benchmarks as they've existed since February 2016;
I did not modify them for this test. My compile lines looked like this:

    make clean
    CXXFLAGS='-std=c++17 -O3 -DNDEBUG -D_LIBCPP_TRIVIALLY_RELOCATABLE=' \
      CXX=../llvm/build/bin/clang++ \
      make -j8 annotate
    ./run-benchmarks.sh

    make clean
    CXXFLAGS='-std=c++17 -O3 -DNDEBUG -DALLMOVES_USE_FLATSET=1' \
      CXX=../llvm/build/bin/clang++ \
      make -j8 annotate
    ./run-benchmarks.sh

    // etc.

`run-benchmarks.sh` is a shell script containing these three lines:

    time for j in `seq 40`; do for i in `seq 40`; do echo ai_move; done | ./annotate --seed $j Sam Dave >/dev/null; done
    time for j in `seq 48`; do ./annotate --auto < benchmarks/perf-27635-moves.txt > /dev/null; done
    time for j in `seq 30`; do ./annotate --auto < benchmarks/perf-33332-moves.txt > /dev/null; done

Here are the results. First the results using `-D_LIBCPP_TRIVIALLY_RELOCATABLE=`, which effectively
disables all P1144-related optimizations. Lower numbers are faster:

| Data structure  | Bench 1 | Bench 2 | Bench 3 |
|:---------------:|:-------:|:-------:|:-------:|
| `unordered_set` |  25.9   |  28.1   |  34.6   |
| `set`           |  29.1   |  34.1   |  41.8   |
| `flat_set`      |  37.6   |  51.9   |  64.0   |

And now without `-D_LIBCPP_TRIVIALLY_RELOCATABLE=`, so that P1144-related optimizations are
enabled. Lower numbers are faster.

| Data structure  | Bench 1 | Bench 2 | Bench 3 |
|:---------------:|:-------:|:-------:|:-------:|
| `unordered_set` |  26.9   |  28.8   |  34.6   |
| `set`           |  29.6   |  35.1   |  40.1   |
| `flat_set`      |  35.5   |  48.8   |  59.9   |

We expect to see improvement anywhere that we use a `swap`-based algorithm (e.g. if we `sort`
things); and anywhere we resize vectors of things (so, we expect the biggest improvement in
the case that's bottlenecked on `flat_set`). We see basically what we expect, plus a good deal
of noise in this very unscientific, population-size-of-1 experiment. (For example, the +1 second
on Benchmark 1 for `unordered_set` is simply noise as far as I know. Running the same benchmark
a second time produced 25.3.)

The `flat_set` numbers were _so bad_ that I went and wrote a not-quite-philosophically-defensible
optimization in `vector::insert`, where if we're inserting just a single element in the middle
of the vector, and trivial relocation is available, then we use `memmove` rather than
move-assign-in-a-loop to do the bulk of the data-shoveling. (If we're inserting more than one
element, then I was too lazy to figure out what the math in libc++'s `__move_range` was doing,
so I just punted on that part.)

With the extra `vector::insert` optimization, `flat_set`'s performance
(on this stupidly unrealistic insertion workload) increases quite a bit. Again, remember there's
tons of noise in here.

| Data structure  | Bench 1 | Bench 2 | Bench 3 |
|:---------------:|:-------:|:-------:|:-------:|
| `unordered_set` |  24.3   |  27.7   |  33.3   |
| `set`           |  29.1   |  33.1   |  39.8   |
| `flat_set`      |  33.2   |  41.1   |  50.3   |

TLDR, libc++ support for `[[trivially_relocatable]]` can give you a maybe 20% speedup on a
real-world benchmark, _if_ that benchmark is the horrible misuse of `flat_set` on an insertion-heavy
workload. On my realistic (non-`flat_set`) workload, my unscientific methods were too noisy
to produce any conclusions of value.
