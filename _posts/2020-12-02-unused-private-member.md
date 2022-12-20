---
layout: post
title: "How different compilers deal with provably unused entities"
date: 2020-12-02 00:01:00 +0000
tags:
  access-control
  compiler-diagnostics
  implementation-divergence
  llvm
  proposal
  sufficiently-smart-compiler
---

The other day, I was asked whether we should worry about the space
taken up by unused functions in our codebase. My answer followed this rambling path:

- Unused function templates won't even get instantiated, so they
    really have no existence at link-time.

- Unused inline functions _may_ be codegenned, but if you have compiler
    optimizations turned on, you probably won't see any codegen for
    inline functions unless they are actually called.

- Unused static non-member functions, logically, should be codegenned,
    but since static functions aren't visible beyond their own
    [TU](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#tu),
    the compiler may be able to detect and warn about them — and
    with compiler optimizations turned on, the compiler may decide
    that there's no reason to generate code for them either.

- Unused non-static (external-linkage) functions, though, _must_ be codegenned,
    because the compiler cannot prove that no other TU calls them.
    An optimizing _linker_ might be able to prove that nobody needs
    those definitions, and eliminate them — I personally have worked
    on a linker that did exactly that optimization — but in general
    your mental model of a linker should be that it won't do that
    optimization.

> The linker is likely able to prove that the function is unused;
> but without some degree of help from the compiler, it probably won't
> be able to prove that it is safe to just snip that function's bytes
> out of the text section. That might mess up some relative offset that
> the rest of the object file is secretly relying on.

"What if they're marked private?"

- Unused private (non-static) data members, logically, must take up space
    in the memory footprint of the class, so that every TU will agree on
    the `sizeof` the class. However, _if_ in some specific TU the compiler
    can see the complete definition of every member and every friend of
    the class, then it can prove that the private member really is
    unused, and then it can give a warning. Clang does this.

- Hmm, but Clang does this _only_ for private data members, not for
    private member functions! That's interesting!

Here's the complete list of non-temploid non-inline entities you might
encounter, and what the major compilers do with them if they're provably
unused. ([Godbolt.](https://godbolt.org/z/cMWPE1))

Options-wise, I tested `-W1 -W2 -W3 -W4 -Od -O1 -O2` on MSVC,
and `-Wall -Wextra -O0 -O1 -O2 -O3` on the other three.

| Do we warn on an unused...                           | Clang   | GCC     | ICC | MSVC  |
|------------------------------------------------------|---------|---------|-----|-------|
| static function                                      | `-Wall` | `-Wall` |     | `-W4` |
| static variable                                      | `-Wall` | `-Wall` |     |       |
| private data member                                  | `-Wall` |         |     |       |
| private static data member                           |         |         |     |       |
| private member function                              |         |         |     |       |
| private static member function                       |         |         |     |       |
| data member of private class                         |         |         |     |       |
| static data member of private class                  |         |         |     |       |
| member function of private class                     |         |         |     |       |
| static member function of private class              |         |         |     |       |
| anonymous-namespaced function                        | `-Wall` | `-Wall` |     |       |
| anonymous-namespaced variable                        | `-Wall` | `-Wall` |     |       |
| data member of anonymous-namespaced class            |         |         |     |       |
| static data member of anonymous-namespaced class     | `-Wall` | `-Wall` |     |       |
| member function of anonymous-namespaced class        |         | `-Wall` |     |       |
| static member function of anonymous-namespaced class |         | `-Wall` |     |       |
| function taking anonymous-namespaced class           | `-Wall` | `-Wall` |     |       |
{:.smaller}

<br>

| Do we optimize out an unused...                      | Clang | GCC   | ICC   | MSVC  |
|------------------------------------------------------|:-----:|:-----:|:-----:|:-----:|
| static function                                      | `-O0` | `-O1` | `-O0` | `-Od` |
| static variable                                      | `-O0` | `-O0` | `-O1` | `-Od` |
| private data member                                  |  —    |  —    |  —    |  —    |
| private static data member                           |  —    |  —    |  —    |  —    |
| private member function                              |  —    |  —    |  —    |  —    |
| private static member function                       |  —    |  —    |  —    |  —    |
| static data member of private class                  |  —    |  —    |  —    |  —    |
| member function of private class                     |  —    |  —    |  —    |  —    |
| static member function of private class              |  —    |  —    |  —    |  —    |
| anonymous-namespaced function                        | `-O0` | `-O1` | `-O0` |       |
| anonymous-namespaced variable                        | `-O0` | `-O0` | `-O1` | `-Od` |
| static data member of anonymous-namespaced class     | `-O0` | `-O0` | `-O1` |       |
| member function of anonymous-namespaced class        | `-O0` | `-O1` | `-O1` |       |
| static member function of anonymous-namespaced class | `-O0` | `-O1` | `-O1` |       |
| function taking anonymous-namespaced class           | `-O0` | `-O1` | `-O1` |       |
{:.smaller}

I claim that there's a fair bit of room for improvement here!

Or am I missing some subtle mechanism by which "unused" private members
might actually be referenced from other TUs?

UPDATE: Yes, I am! See my followup:

* ["How to use a private member from outside the class"](/blog/2020/12/03/steal-a-private-member)

I have placed "–" in table cells where this loophole makes the suggested optimization
technically impossible for a conforming compiler. All compilers' diagnostics could
use improvement, but the new optimization table shows that everyone (except MSVC)
is doing the best they can, optimization-wise.
