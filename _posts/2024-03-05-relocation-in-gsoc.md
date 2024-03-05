---
layout: post
title: 'The 2024 Google Summer of Code idea lists are out'
date: 2024-03-05 00:01:00 +0000
tags:
  conferences
  relocatability
excerpt: |
  As of late February, [Google Summer of Code](https://opensource.googleblog.com/2024/02/mentor-organizations-announced-for-google-summer-of-code-2024.html)
  (GSoC) has published its official list of sponsoring open-source organizations, and each organization
  has independently published its own list of project ideas. People interested in doing a GSoC project
  (sort of a summer internship for open source software) should
  [submit a proposal application](https://summerofcode.withgoogle.com/how-it-works) —
  the quite short application window is March 18 to April 2.

  If you're a fan of my "trivial relocatability" content, or just want to help relocation's
  progress into the mainstream of C++, you might be particularly interested in these
  three GSoC sponsors:
---

As of late February, [Google Summer of Code](https://opensource.googleblog.com/2024/02/mentor-organizations-announced-for-google-summer-of-code-2024.html)
(GSoC) has published its official list of sponsoring open-source organizations, and each organization
has independently published its own list of project ideas. People interested in doing a GSoC project
(sort of a summer internship for open source software) should
[submit a proposal application](https://summerofcode.withgoogle.com/how-it-works) —
the quite short application window is March 18 to April 2.

If you're a fan of my "trivial relocatability" content, or just want to help relocation's
progress into the mainstream of C++, you might be particularly interested in these
three GSoC sponsors:

- [Ste||ar HPX](https://summerofcode.withgoogle.com/programs/2024/organizations/stear-group).
    Their [ideas list](https://github.com/STEllAR-GROUP/hpx/wiki/Google-Summer-of-Code-%28GSoC%29-2024)
    includes a sequel to [last summer's successful project](https://isidorostsa.github.io/gsoc2023/) implementing `hpx::uninitialized_relocate`;
    this summer's goal is [to parallelize `hpx::uninitialized_relocate`](https://github.com/STEllAR-GROUP/hpx/wiki/Google-Summer-of-Code-%28GSoC%29-2024#implement-parallel-hpxuninitialized_relocate_-algorithms-for-overlapping-ranges)
    (and perhaps also `hpx::copy`?) for ranges that overlap. Parallelizable relocation is a building block
    for parallel containers such as ParlayLib's [`sequence<T>`](https://cmuparlay.github.io/parlaylib/datatypes/sequence.html).

- [GCC](https://summerofcode.withgoogle.com/programs/2024/organizations/gnu-compiler-collection-gcc).
    Their [ideas list](https://gcc.gnu.org/wiki/SummerOfCode) doesn't include anything relocation-related —
    but you could propose something! For example: Adding `__is_trivially_relocatable(T)` to the compiler
    along [these lines](https://github.com/gcc-mirror/gcc/commit/5742fba8b43d1ada136f01efd05b03276e1bcc04) but better.
    Implementing a way to mark types such as `deque` trivially relocatable (i.e. `[[gnu::trivially_relocatable]]`).
    Rewriting libstdc++'s [`__is_bitwise_relocatable`](https://github.com/gcc-mirror/gcc/blob/a945c34/libstdc++-v3/include/bits/stl_deque.h#L2374-L2380)
    in terms of your new `__is_trivially_relocatable(T)`, so that aggregates _containing_ deques
    get the same benefit as `deque` itself. ([Godbolt.](https://godbolt.org/z/oovrEqGnn))

- [LLVM/Clang](https://summerofcode.withgoogle.com/programs/2024/organizations/llvm-compiler-infrastructure).
    Their [ideas list](https://llvm.org/OpenProjects.html) doesn't include anything relocation-related —
    but you could propose something! For example: Clang provides an `__is_trivially_relocatable(T)` builtin,
    but it often gives wrong answers ([#69394](https://github.com/llvm/llvm-project/issues/69394),
    [#77091](https://github.com/llvm/llvm-project/issues/77091)). Fixing this would allow projects such as
    [Folly](https://github.com/facebook/folly/blob/65fb952/folly/Traits.h#L635-L641),
    [Abseil](https://github.com/abseil/abseil-cpp/blob/14b8a4e/absl/meta/type_traits.h#L498-L536),
    [Qt](https://github.com/qt/qtbase/blob/a6b4ff16/src/corelib/global/qtypeinfo.h#L25-L26)
    to start conditionally using Clang's builtin, say, in the Clang 19 timeframe.
    You could also implement relocation optimizations in libc++'s `vector` and `swap_ranges`.

Again, the application window for GSoC participants to [submit a proposal application](https://summerofcode.withgoogle.com/how-it-works)
opens on March 18 and closes on April 2.
