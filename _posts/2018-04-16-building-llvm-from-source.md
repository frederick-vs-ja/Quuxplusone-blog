---
layout: post
title: 'How to build LLVM from source'
date: 2018-04-16 00:02:00 +0000
tags:
  how-to
  llvm
---

<b>UPDATE, November 2019: The old llvm-mirror is dead; long live
the monorepo!</b> Updated instructions on how to build LLVM from source are
available in [this later blog post](/blog/2019/11/09/llvm-from-scratch).

----

There are two plausible sources of the LLVM codebase:
`https://llvm.org/git/$FOO.git` and `https://github.com/llvm-mirror/$FOO.git`.
The GitHub mirror seems to be faster than `llvm.org`; plus, we're
going to want to fork it on GitHub to track our changes.

(However, to submit patches to LLVM projects, you must use
[the official Phabricator](https://reviews.llvm.org/differential/diff/create/);
don't submit GitHub pull requests against the `llvm-mirror` mirrors!)


## Step 1: Fork!

Go to your GitHub account and fork each of the following repos:

- [llvm-mirror/llvm](https://github.com/llvm-mirror/llvm)
- [llvm-mirror/clang](https://github.com/llvm-mirror/clang)


## Step 2: Get the code!

Locally clone the repos to the right places.

    cd $ROOT               ; git clone git@github.com:llvm-mirror/llvm
    cd $ROOT/llvm/tools    ; git clone git@github.com:llvm-mirror/clang

Here are the right places for some other useful (but unnecessary) repos.

    cd $ROOT/llvm/projects          ; git clone git@github.com:llvm-mirror/libcxx
    cd $ROOT/llvm/projects          ; git clone git@github.com:llvm-mirror/compiler-rt
    cd $ROOT/llvm/tools             ; git clone git@github.com:llvm-mirror/lldb
    cd $ROOT/llvm/tools/clang/tools ; git clone git@github.com:llvm-mirror/clang-tools-extra extra

(Notice that `clang-tools-extra`'s repo name doesn't match its expected directory name.
We snuck an `extra` parameter onto the end of that `git clone` line.)

This is a good time to set up the `.git/config` for each of the repos
you just cloned (for example, `$ROOT/llvm/.git/config`).
I set it up this way:

    [remote "origin"]
        url = git@github.com:Quuxplusone/llvm.git
        fetch = +refs/heads/*:refs/remotes/origin/*
    [remote "upstream"]
        url = git@github.com:llvm-mirror/llvm.git
        fetch = +refs/heads/*:refs/remotes/upstream/*
    [branch "master"]
        remote = upstream
        merge = refs/heads/master

This gives me two remotes: one named `upstream` whence I can pull,
and one named `origin` whither I can push. My local `master` and `origin/master`
will both track `upstream/master`. Anything I do in my local repo, I will do in a
feature branch; my feature branches will track `origin`.


## Step 3: Build!

    mkdir -p $ROOT/llvm/build
    cd $ROOT/llvm/build
    cmake -G 'Unix Makefiles' \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
    make -j5 clang
    make -j5 check-clang

If you omit `-DCMAKE_BUILD_TYPE=RelWithDebInfo`, this first part will still work, but you'll produce
a "debug-build" version of `clang` that is super slow, and then the "bootstrap" step below will
take days instead of minutes. So watch out for that.

`make -j5 clang` takes about 80 minutes on my laptop.
`make -j5 check-clang` takes another 37 minutes
(27 minutes to build `clang-tidy` for some reason, and then another 10 minutes to run the actual tests).

Making `clang` will build both `clang` and `clang++`.
Making `cxx` will build `libc++` (and also `libc++abi`, which is included in the `libcxx` repo).
Making `check-$FOO` will build and run the test suite for `$FOO`:

    make -j5 check-clang check-cxx

If something goes wrong, you can usually recover via

    rm $ROOT/llvm/build/CMakeCache.txt

and, absolute worst case, you can blow away `$ROOT/llvm/build` and start over.


## Step 4: Run specific tests.

Running a specific test or directory-of-tests for any product is easy:

    cd $ROOT/llvm/build
    ./bin/llvm-lit -sv ../test/Analysis
    ./bin/llvm-lit -sv ../tools/clang/test/ARCMT
    ./bin/llvm-lit -sv ../projects/libcxx/test/std/re

However, it looks like before you can successfully run one of these lines,
you must have run the corresponding one of `make check-{llvm,clang,cxx}`
at least once, to initialize the right stuff in the `build` directory.

(Thanks to [Brian Cain](http://lists.llvm.org/pipermail/llvm-dev/2018-May/123049.html)
for documenting this recipe.)

But watch out! Both `make check-cxx` and `llvm-lit` will by default use your *system compiler*
to run the libc++ tests! This is not what you want! Tell `llvm-lit` to use your newly built Clang
by passing the `cxx_under_test` parameter, like this:

    ./bin/llvm-lit -sv --param cxx_under_test=`pwd`/bin/clang ../projects/libcxx/test/


## Step 5: Bootstrap!

This is where it might get non-portable for people who aren't on OS X, I'm not sure.
Here we will *not* be installing Clang over top of the system compiler
(super dangerous!); but we *will* instruct CMake to build Clang using
the previously built Clang.

There is apparently [an official way to bootstrap Clang](https://llvm.org/docs/AdvancedBuilds.html):

    cd $ROOT/llvm/build
    make -j5 clang
    cmake -G 'Unix Makefiles' \
        -DCLANG_ENABLE_BOOTSTRAP=On \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
    make -j5 stage2

However, when I get to `make -j5 stage2`, it fails with a CMake error:

    Host Clang must be able to find libstdc++4.8 or newer!

So when I bootstrap Clang, I use this crude approach inspired by
[the CMake FAQ](https://gitlab.kitware.com/cmake/community/wikis/FAQ#how-do-i-use-a-different-compiler):

    cd $ROOT/llvm/build
    rm CMakeCache.txt
    CXX="$ROOT/llvm/build/bin/clang++" \
    CXXFLAGS="-cxx-isystem /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1 -isystem /usr/include" \
    cmake -G 'Unix Makefiles' \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
    cp bin/clang-8 clang-ok
    find ../include/ -name '*.h' | xargs touch
    make -j5 clang VERBOSE=1

(This takes the same 80 minutes as the original `make -j5 clang` did.)

(If you have checked out `libcxx`, you can try substituting
`$ROOT/llvm/projects/libcxx/include` for `/Library/Developer/CommandLineTools/usr/include/c++/v1`.)

This crude approach will of course overwrite your "good" `build/bin/clang++`
(the one that successfully compiled Clang) with a new version (which for all
you know might *not* compile Clang), so that's why I did `cp bin/clang-8 clang-ok`
before running `make -j5 clang` the second time.
(Make sure you `cp` the actual executable and not just a symlink to it!)
