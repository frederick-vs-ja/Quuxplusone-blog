---
layout: post
title: "Refactoring with `=delete`"
date: 2022-11-11 00:01:00 +0000
tags:
  c++-learner-track
  overload-resolution
  war-stories
excerpt: |
  Pro tip: I've found `=delete` quite a useful tool when refactoring overly complicated overload sets
  in legacy code. Suppose we have an overload set in some legacy code that looks like this:

      void foo(const char*, int, bool = false);  // #1
      void foo(const char*, bool = false);       // #2
      void foo(int, bool = false);               // #3

  We suspect that overload #1 is unused. We comment it out, and the codebase still compiles,
  so we think it's safe to remove.
---

Previously on this blog:

* ["A very short war story on too much overloading"](/blog/2020/10/11/overloading-considered-harmful/) (2020-10-11)
* ["What `=delete` means"](/blog/2021/10/17/equals-delete-means/) (2021-10-17)
* ["When Should You Give Two Things the Same Name?"](https://www.youtube.com/watch?v=OQgFEkgKx2s) (C++Now 2021)

Pro tip: I've found `=delete` quite a useful tool when refactoring overly complicated overload sets
in legacy code. Suppose we have an overload set in some legacy code that looks like this:

    void foo(const char*, int, bool = false);  // #1
    void foo(const char*, bool = false);       // #2
    void foo(int, bool = false);               // #3

We suspect that overload #1 is unused. We comment it out, and the codebase still compiles,
so we think it's safe to remove.

But by removing it we introduced a bug! One of our other files has a call-site like this:

    foo("xyzzy", 42);

This used to call #1 as the best match; but with #1 eliminated, it will silently call #2 instead.
The "right" way to test whether #1 is unused is not to comment it out, but to mark it `=delete`.
This makes all its callers ill-formed without changing the overload resolution, so we can be sure
we're seeing the complete set of all the call-sites of this specific overload. (We hope that that
set is empty, so that we can safely remove the overload.)

----

Now, each overload above actually corresponds to two signatures, and it might be that one of those
two signatures is unused while the other isn't.
(See ["Default function arguments are the devil"](/blog/2020/04/18/default-function-arguments-are-the-devil/) (2020-04-18).)
For example, suppose I hypothesize that overload #3's default argument is never used.
I can't test that by simply removing the `bool` parameter's default value, because that would
change the overload resolution for this call-site:

    foo(0);

This used to call #3 as the best match; but with #3's default argument removed, it will silently
call #2 instead.

So actually, even before I started `=delete`’ing, I would refactor the above overload set into
one overload per signature (with appropriate `inline` markings if needed — not shown):

    void foo(const char *, int, bool);  // #1a
    void foo(const char *s, int i) { return foo(s, i, false); }  // #1b
    void foo(const char *, bool);  // #2a
    void foo(const char *s) { return foo(s, false); }  // #2b
    void foo(int, bool);  // #3a
    void foo(int i) { return foo(i, false); }  // #3b

Then I could safely try `=delete`’ing the one-argument signature #3b
while leaving #3a alone.

----

See also:

* ["Overload arrangement puzzles"](/blog/2021/10/29/overload-arrangement-puzzles/) (2021-10-29)
