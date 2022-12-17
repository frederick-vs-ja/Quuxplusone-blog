---
layout: post
title: "`while (auto x=y; z)`"
date: 2020-10-28 00:01:00 +0000
tags:
  language-design
  cpplang-slack
---

This question has come up at least twice on the cpplang Slack now. C++ keeps
adding more and more knobs to `if` and `for` and `switch`; why hasn't it
messed with `while`? Specifically, why isn't there a "two-part while loop"?

C++ offers the following control structures (where "`init`" represents
the choice of either a declaration or an expression-statement):

    if (z)                     // '98
    if (auto z=w)              // '98
    if (init; z)               // C++17
    if (init; auto z=w)        // C++17

    if constexpr (z)           // all C++17
    if constexpr (auto z=w)
    if constexpr (init; z)
    if constexpr (init; auto z=w)

    for (init; z; ++x)         // '98
    for (init; auto z=w; ++x)  // '98
    for (auto e : r)           // C++11
    for (init; auto e : r)     // C++20

    while (z)                  // '98
    while (auto z=w)           // '98
    do { ... } while (z)       // '98

    switch (z)                 // '98
    switch (auto z=w)          // '98
    switch (init; z)           // C++17
    switch (init; auto z=w)    // C++17

Notably missing from the middle of this list: `while (init; z)` and
`while (init; auto z=w)`.

The reason `while (init; cond)` is missing is that there are two
reasonable interpretations of what it might mean. The Committee could
pick one behavior to be "correct"; but if they did that, programmers
would inevitably write the construct expecting the _other_ behavior,
and then they'd have bugs.


## Option 1: Evaluate the init only once

Kirit Sælensminde offered the following use-case:

    auto cursor = getCursor();
    while (auto item = cursor.next()) {
        use(item);
    }

If this is your use-case, then it might seem unfortunate that you can't
combine the declaration of `cursor` into the `while`-loop; it has to
"leak" into the outer scope. You might _want_ to write

    while (auto c = getCursor(); auto item = c.next()) {
        use(item);
    }

It turns out that you can actually write this loop in a way
that's just as short, and arguably clearer (since it uses only
C++98 features): just use a `for` loop instead!

    for (auto c = getCursor(); auto item = c.next(); ) {
        use(item);
    }


## Option 2: Evaluate the init every time

The other use-case for which you might want a two-part `while` loop is:

    int ch;
    while ((ch = getchar()) != EOF) {
        use(ch);
    }

If this is your use-case, then it might seem unfortunate that you can't
combine the declaration of `ch` into the `while`-loop; it has to
"leak" into the outer scope. (And go uninitialized, too!) You might _want_ to write

    while (int ch = getchar(); ch != EOF) {
        use(ch);
    }

I'm not aware of any clean way to write this loop that avoids leaking
`ch` into the outer scope. I mean, I don't consider any of these "clean":

    for (int ch; (ch = getchar()) != EOF; ) {
        use(ch);
    }

    for (int ch = getchar(); ch != EOF; ch = getchar()) {
        use(ch);
    }

    while (true) {
        if (int ch = getchar(); ch != EOF) {  // C++17
            use(ch);
        } else {
            break;
        }
    }

----

So, given that there's a clean way to write Option 1 already, and no equally clean way
to write Option 2, shouldn't the Committee simply add Option 2 to the language? **No.**

We can't give programmers the ability to write

    while (int ch = getchar(); ch != EOF) { ... }

without _also_ giving them the ability to shoot themselves in the foot with

    while (auto c = getCursor(); auto item = c.next()) {
        // repeatedly fetch the list and process just its first item,
        // over and over, forever
    }

I'm happy to keep programming without a complicated "two-part `while` loop,"
if it means that other programmers are happily prevented from shooting themselves
in the foot.
