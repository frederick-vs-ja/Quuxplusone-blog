---
layout: post
title: "Hash-colliding string literals on MSVC"
date: 2022-12-31 00:01:00 +0000
tags:
  implementation-divergence
  msvc
---

In my never-ending quest to make C++ braced initializers
`{1,2,3}` behave more like string literals `"abc"`
(see ["`span` should have a converting constructor from `initializer_list`"](/blog/2021/10/03/p2447-span-from-initializer-list/) (2021-10-03),
which became [P2447](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2447r2.html);
and see my upcoming [D2752 "Static storage for braced initializers"](https://quuxplusone.github.io/draft/d2752-static-storage-for-braced-initializers.html)),
I've been looking at how different compilers mangle the symbols for
hidden variables such as the guard variables of static locals,
the backing variables of structured bindings, and so on.
This also led me to look at how the character arrays backing
string literals are emitted by different compilers.

MSVC at `-O1` and higher does something weirdly unnecessary:
it gives string literals externally visible names which incorporate
a short hash of the string's contents. Short hashes means hash collisions.
Thus ([Godbolt](https://godbolt.org/z/E7eob9vcM)):

    #include <stdio.h>

    int main() {
      puts("A banner with the strange device 'Migicative'!");
      puts("A banner with the strange device 'Borabigate'!");
    }

On MSVC with `-O1` or higher, this prints the first string twice.

> UPDATE, 2023-01-01: David Bakin writes to tell me that MSVC calls this feature
> ["String Pooling"](https://learn.microsoft.com/en-us/cpp/build/reference/gf-eliminate-duplicate-strings)
> and that although it defaults to "on" at `-O1` and higher, it can be toggled
> on and off with the command-line switch `-GF`/`-GF-`.
>
> MSVC's string pooling piggybacks on the existing COMDAT mechanism, which was
> designed for inline functions and variables: situations where you need
> to retain only one copy of a function with a given symbol name, and you expect
> different copies to have different contents due to optimization level and so on.
> String-pooling has exactly the opposite requirements: strings are anonymous,
> and you care deeply about their contents.
>
> The best way to do string pooling, in my opinion (and in my experience at
> Green Hills over a decade ago), is to have the compiler mark anonymous entities
> whose addresses needn't be unique and then authorize the linker to lay out the
> `.rodata` section in the most compacted way, based on content rather than
> on symbol or section name. MSVC's symbol-based approach might be defended as
> a decent approximation to content-addressing, that works without modification
> to their present-day ('80s-era) linker — attacked as an example of moving work
> from the tool that is ideally suited to that work, into a different tool
> where it costs more to do a poorer job — or merely presented as a curiosity.
