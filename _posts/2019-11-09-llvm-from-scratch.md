---
layout: post
title: 'How to build LLVM from source, monorepo version'
date: 2019-11-09 00:01:00 +0000
tags:
  how-to
  llvm
---

This is an update of my previous post
["How to build LLVM from source"](/blog/2018/04/16/building-llvm-from-source) (2018-04-16),
which is now out-of-date. LLVM has moved to a "monorepo" design instead of a collection
of smaller tool-specific repositories. So this new post has fewer steps!

UPDATE, 2020-03-22: Patch [D69221](https://reviews.llvm.org/D69221) seems to have made
the procedure much simpler, eliminating the need for Jens Jorgensen's patch,
at least on OSX 10.14.6. Therefore I have shortened this post considerably.
You can find the old version [in the blog's git history](https://github.com/Quuxplusone/blog/commits/master/_posts/2019-11-09-llvm-from-scratch.md)
or [on the Wayback Machine](https://web.archive.org/web/20200323024244/https://quuxplusone.github.io/blog/2019/11/09/llvm-from-scratch/).

UPDATE, 2024-11-24: Some LLVM subprojects are now called "runtimes" rather than "projects";
the CMake flags have changed accordingly. Also, [since October 2024](https://github.com/llvm/llvm-project/issues/92121#issuecomment-2198511835),
`-DLIBCXXABI_USE_LLVM_UNWINDER=OFF` and `-DCOMPILER_RT_USE_LLVM_UNWINDER=OFF` seem necessary
in order to make exception-handling work on an ARM MacBook.

----

The LLVM codebase's official home is [`https://github.com/llvm/llvm-project`](https://github.com/llvm/llvm-project).

(However, to submit patches to LLVM projects, you must use
[the official Phabricator](https://reviews.llvm.org/differential/diff/create/);
don't submit GitHub pull requests against `llvm/llvm-project`!
At least not as of December 2020.)


## Step 1: Fork!

Go to your GitHub account and fork the following repository:

- [llvm/llvm-project](https://github.com/llvm/llvm-project)


## Step 2: Get the code!

Locally clone the repo to the right place.

    cd $ROOT
    git clone git@github.com:llvm/llvm-project

This is a good time to set up the `.git/config` for the repo
you just cloned (for example, `$ROOT/llvm-project/.git/config`).
I set it up this way:

    [remote "origin"]
        url = git@github.com:Quuxplusone/llvm-project.git
        fetch = +refs/heads/*:refs/remotes/origin/*
    [remote "upstream"]
        url = git@github.com:llvm/llvm-project.git
        fetch = +refs/heads/*:refs/remotes/upstream/*
    [branch "master"]
        remote = upstream
        merge = refs/heads/master

This gives me two remotes: one named `upstream` whence I can pull,
and one named `origin` whither I can push. My local `master` and `origin/master`
will both track `upstream/master`. Anything I do in my local repo, I will do in a
feature branch; my feature branches will track `origin`.


## Step 3: Build!

    mkdir $ROOT/llvm-project/build
    cd $ROOT/llvm-project/build
    cmake -G Ninja \
        -DDEFAULT_SYSROOT="$(xcrun --show-sdk-path)" \
        -DLLVM_ENABLE_PROJECTS="clang" \
        -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
        -DCMAKE_BUILD_TYPE=Release ../llvm
    ninja clang
    ninja cxx

Making `clang` will build both `clang` and `clang++`.

If you omit `-DCMAKE_BUILD_TYPE=Release` (or at least `-DCMAKE_BUILD_TYPE=RelWithDebInfo`),
this first part will still work, but you'll produce
a "debug-build" version of `clang` that is super slow, and then the "bootstrap" step below will
take days instead of minutes. So watch out for that.

On my laptop, `cmake` takes about 42 seconds;
`ninja clang` takes about 96 minutes.


### Troubleshooting step 3

If something goes wrong, you can usually recover via

    rm $ROOT/llvm-project/build/CMakeCache.txt

and, absolute worst case, you can `rm -rf $ROOT/llvm-project/build` and start over.

----

If you succeed in building `clang`, but then when you run it you get errors about
the standard C-language headers, like this,

    $ bin/clang++ test.cpp
    test.cpp:1:10: fatal error: 'stdio.h' file not found
    #include <stdio.h>
             ^~~~~~~~~
    1 error generated.

then you may have set `DEFAULT_SYSROOT` inappropriately.
On Sonoma 14.7, when I run `xcrun --show-sdk-path`, I get `/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk`.
Your results may vary.

If you get errors about the C++ headers, such as `<vector>`, it's because you still
need to build libc++: run `ninja cxx`.


## Step 4: Bootstrap `check-clang` and libc++.

Here we will instruct CMake to build Clang again, using the Clang we just built.
There is apparently [an official way to bootstrap Clang](https://llvm.org/docs/AdvancedBuilds.html)
(probably out-of-date). However, I use an approach inspired by
[the CMake FAQ](https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#how-do-i-use-a-different-compiler).
Note that we will *not* be installing Clang over top of the system compiler; that would be super dangerous
and you should never do it!

    mkdir $ROOT/llvm-project/build2
    cd $ROOT/llvm-project/build2
    CXX="$ROOT/llvm-project/build/bin/clang++" \
    cmake -G Ninja \
        -DDEFAULT_SYSROOT="$(xcrun --show-sdk-path)" \
        -DLLVM_ENABLE_PROJECTS="clang;compiler-rt" \
        -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
        -DLIBCXXABI_USE_LLVM_UNWINDER=OFF \
        -DCOMPILER_RT_USE_LLVM_UNWINDER=OFF \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo ../llvm
    ninja clang cxx
    ninja check-clang check-cxx

Now you have built two versions of `clang++`: `$ROOT/llvm-project/build/bin/clang++` is the version
built with your system compiler, and `$ROOT/llvm-project/build2/bin/clang++` is the version built with
_that_ version. You can extend this to `build3`, `build4`, etc.

Making `cxx` will build `libc++.dylib`, `libc++.a`, and `libc++abi.dylib`.
Making `cxxabi` will build `libc++abi.a`.
Making `check-cxx` will build `libc++experimental.a`.
Making `compiler-rt` will build `libclang_rt.asan_osx_dynamic.dylib` and `libclang_rt.ubsan_osx_dynamic.dylib`,
which are needed by Clang's `-fsanitize=address` and `-fsanitize=undefined` options.

This time, `cmake` takes about 58 seconds; `ninja clang` takes about 153 minutes.
`ninja check-clang` takes another 53 minutes:
36 minutes to build a bunch of additional tools, and then 17 minutes to run the actual tests.
`ninja cxx` takes about 84 seconds.
`ninja check-cxx` takes about 50 minutes (but see the caveat below about `cxx_under_test`).

You can invoke CMake with `-DCMAKE_BUILD_TYPE=Debug` to produce a `clang` binary with assertions enabled.
This takes only about 60 minutes to build, but you wouldn't want to use the resulting binary for
anything heavy-duty because it's so slow.


## Step 5: Run specific tests.

Running a specific test or directory-of-tests for any product is easy:

    cd $ROOT/llvm-project/build2
    ./bin/llvm-lit -sv ../llvm/test/Analysis
    ./bin/llvm-lit -sv ../clang/test/ARCMT
    ./bin/llvm-lit -sv --param std=c++17 ../libcxx/test/std/re

UPDATE, 2024: The last of these lines no longer works. Libc++ is now a "runtime," which means you
must run its tests as if they live in a different directory within the buildroot itself. This
directory doesn't actually exist, but you must write your paths as if it does:

    ./bin/llvm-lit -sv --param std=c++17 ./runtimes/runtimes-bins/libcxx/test/std/re

Also note that before you can successfully run any of these lines,
you must have run the corresponding one of `make check-{llvm,clang,cxx}`
at least once, to initialize the right stuff under the `build2` directory.

(Thanks to [Brian Cain](http://lists.llvm.org/pipermail/llvm-dev/2018-May/123049.html)
for documenting this recipe.)

But watch out — both `make check-cxx` and `llvm-lit` will by default use your *system compiler*
to run the libc++ tests! This is not what you want! Tell `llvm-lit` to use your newly built Clang
by passing the `cxx_under_test` parameter, like this:

    ./bin/llvm-lit -sv --param std=c++17 --param cxx_under_test=`pwd`/bin/clang \
        ./runtimes/runtimes-bins/libcxx/test

On my laptop, this command line again takes about 50 minutes to run all the libc++ tests,
but this time it correctly uses the bootstrapped compiler.
