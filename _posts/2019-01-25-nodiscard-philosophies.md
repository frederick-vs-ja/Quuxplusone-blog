---
layout: post
title: "`nodiscard` philosophies"
date: 2019-01-25 00:01:00 +0000
tags:
  attributes
  compiler-diagnostics
  library-design
  nodiscard
---

GCC's `warn_unused_result` attribute has gotten a bad reputation by being, essentially,
broken for a decade or two — and it is still broken, as of this writing.
[There is no way to suppress the resulting diagnostic](https://godbolt.org/z/wwjxLP).
This led to several bug reports against the GCC compiler:

- [Bug 66425 "`(void)` cast doesn't suppress `__attribute__((warn_unused_result))`"](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66425) (June 2015, as yet UNCONFIRMED)

- [Bug 25509 "can't disable `__attribute__((warn_unused_result))`"](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=25509) (December 2005, resolved FIXED)

And it led to library writers avoiding `__attribute__((warn_unused_result))` — even the writers of glibc itself!

- [Bug 11959 "fwrite must not be declared with the warn_unused_result attribute"](https://sourceware.org/bugzilla/show_bug.cgi?id=11959) (August 2010, resolved FIXED)

Plenty of open-source projects use ugly workarounds for the GCC issue, such as rewriting

    write(mainpipe[1], "", 1);

into

    if (write(mainpipe[1], "", 1)) {}

(an example taken directly from [DNS-OARC's `dnsperf`](https://github.com/DNS-OARC/dnsperf/commit/b84e41b0c42ccbf40c4b231ffbc213fbefd569be#diff-3b43edd69582341ecfca9aae25e25467R190),
but for the moral equivalent, see [Mozilla](https://hg.mozilla.org/mozilla-central/rev/96c341954032)).

Another approach is to introduce your own non-annotated helper function
(such as [`write_wrapped` in hellerve/e](https://github.com/hellerve/e/pull/12/files)).

----

It strikes me that there are two different rationales for "why to mark things nodiscard."

The "GCC rationale": Ignoring this return value will lead to security vulnerabilities! You must **always**
check the result of this function or else your program has a security hole. We don't care what your _intent_
was; if you meant to discard this result, then _your intent was wrong._ You must never ever fail to check
the result of this function. If we catch you trying to ignore the result, we will refuse to compile your program.
We would rather have _no_ executable than contribute to the production of an _insecure_ executable!

The "MSVC rationale": Ignoring this return value is probably a mistake. You didn't really mean to discard
this result, did you? Your code probably doesn't match your intent, and you should take another look at it
for typos. If you really meant to discard this result, then okay, but please cast it to `void` so we know
not to bother you again.

Personally, I think the "MSVC rationale" is much closer to a helpful reality than the "GCC rationale." 
We should help the programmer write the code that best matches their intent, not complain that the
programmer's intent was wrong.

----

The C++17 standard attribute [`[[nodiscard]]`](https://en.cppreference.com/w/cpp/language/attributes/nodiscard) 
explicitly follows the MSVC rationale: casting to `void` is the normatively blessed way to suppress the
resulting diagnostic.

[GCC correctly respects `(void)` when the attribute is spelled `[[nodiscard]]`](https://godbolt.org/z/Yp19sS);
its bad behavior applies _only_ to the non-standard `__attribute__((warn_unused_result))`.
Full disclosure: Intel ICC never respects `(void)`, not even for the standard attribute.

    __attribute__((warn_unused_result))
    int wur();

    [[nodiscard]]
    int nd();

    int should_warn() {
        wur();  // OK, warns
        nd();   // OK, warns
        return 0;
    }

    int void_cast_should_not_warn() {
        (void)wur();  // OOPS, warns on GCC (but not on Clang)
        (void)nd();   // OK, doesn't warn
        return 0;
    }
