---
layout: post
title: "C++2a idioms for library feature detection"
date: 2018-10-26 00:02:00 +0000
tags:
  c++-style
  feature-test-macros
---

The C++2a standard will introduce the `<version>` header, which contains all
the "feature macros" indicating features supported by the standard library.
So if you live in the year 2035 and compile for a lot of different compilers,
all of which support at least the bare bones of C++2a, `<version>` might
soon be your best friend.

Suppose C++2b introduces `std::experimental::superoptional<T>`, which is like
`optional` but even better; and then C++2c moves it from `std::experimental`
into the main `std` namespace. Our future self will write this code:

    #include <version>  // must exist because C++2a
    #if __cpp_lib_superoptional >= 202601L
     #include <superoptional>
     namespace super = std;
    #elif __cpp_lib_experimental_superoptional >= 202301L
     #include <experimental/superoptional>
     namespace super = std::experimental;
    #else
     #error "I didn't find anything appropriate for namespace super"
    #endif

and now we can use `super::superoptional<T>`.

In C++2a, including any library header (other than `<version>`) prior to testing the
corresponding `__cpp_lib_feature_flag` is an antipattern, as far as I can tell —
because `#include`ing an unsupported header is allowed to blow up, and [frequently
*does* blow up in practice](https://godbolt.org/z/PmDbHI).
So you have to test first and `#include` second.

Testing `__has_include(<any_header>)` is redundant for any standard header whose
existence is implied by a `__cpp_lib_feature_flag`; you can just test the feature
flag instead.

Testing `__has_include()` will still be useful if you're checking for
a non-standard header, e.g.

    #include <version>
    #if __cpp_lib_superoptional >= 202601L
     #include <superoptional>
     namespace super = std;
    #elif __cpp_lib_experimental_superoptional >= 202301L
     #include <experimental/superoptional>
     namespace super = std::experimental;
    #elif __has_include(<boost/superoptional.hpp>)
     #include <boost/superoptional.hpp>
     namespace super = boost::superoptional;
    #else
     #error "I didn't find anything appropriate for namespace super"
    #endif

----

Of course this new idiom works only for C++2a-and-later programmers, which is
none of us right now... but by the year 2035 will be most of us.

One action you can take today to prepare for C++2a is to audit your
codebase's include directories for any text files named `VERSION` and
move those files elsewhere!
