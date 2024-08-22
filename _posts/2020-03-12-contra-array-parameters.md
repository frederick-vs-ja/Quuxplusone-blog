---
layout: post
title: '_Contra_ `char *argv[]`'
date: 2020-03-12 00:01:00 +0000
tags:
  mailing-list-quotes
  pitfalls
  rant
---

Language warning.

Linus Torvalds hits the nail on the head in [this message](https://lkml.org/lkml/2015/9/3/428)
from the Linux kernel mailing list (September 2015). He's talking about a buggy refactoring
(made [here](https://github.com/torvalds/linux/commit/90c66bd2232ae6d3c88c1f3378e3028fded642b3#diff-ca015ca518ca849f187103fe395cee7cR620),
fixed [here](https://github.com/torvalds/linux/commit/98a1f8282b8c37378c1b947d661a58942331ca90))
which essentially refactored

    void test() {
        char mcs_mask[IEEE80211_HT_MCS_MASK_LEN];
        for (int i=0; i < sizeof(mcs_mask); ++i) {  // sketchy but correct
            mcs_mask[i] &= SOME_MASK;
        }
    }

into

    void helper(char mcs_mask[IEEE80211_HT_MCS_MASK_LEN]) {
        for (int i=0; i < sizeof(mcs_mask); ++i) {  // totally wrong
            mcs_mask[i] &= SOME_MASK;
        }
    }

    void test() {
        char mcs_mask[IEEE80211_HT_MCS_MASK_LEN];
        helper(mcs_mask);
    }

Linus writes:

> I [really] want people to take a really hard look at functions that use
> arrays as arguments. It really is very misleading, even if it can look
> "prettier", and some people will argue that it's "documentation" about
> how the pointer is a particular size. But it's neither. It's basically
> just lying about what is going on, and the only thing it documents is
> "I don't know how to C". Misleading documentation isn't documentation,
> it's a mistake.

Please, don't use `[]` syntax to declare pointers.

----

Previously on this blog:

* ["_Contra_ locales"](/blog/2018/04/30/contra-locales/) (2018-04-30)
