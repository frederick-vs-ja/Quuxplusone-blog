---
layout: post
title: "Always read the first error message first"
date: 2023-01-21 00:01:00 +0000
tags:
  c++-learner-track
  compiler-diagnostics
---

In training classes, one of the things I always say (besides "Ask questions!")
is "Always look at the first error message, not the last one." It's so tempting
to look at the last line of the compiler's output first: after all, it's the one
that's right there in front of you when the compiler finishes running. You think,
"Well, I need to fix all the errors at some point anyway; might as well start here."
_It's a trap!_ Suppress that temptation, and scroll up to the first error message first.

> Sometimes students on Linux will say, "I can't see the first error message because
> my terminal's scrollback buffer isn't big enough." There's usually a way to increase the
> size of that buffer, although the non-GUI methods sound arcane
> ([1](https://superuser.com/questions/188993/how-do-i-increase-terminal-scrollback-buffer-size),
> [2](https://unix.stackexchange.com/questions/346018/how-to-increase-the-scrollback-buffer-size-for-tty)).
> You can try `tmux` or `screen`, which have [their own problems](https://unix.stackexchange.com/questions/18006/can-mouse-wheel-scrolling-work-in-a-screen-session).
> As a last resort, [pipe](https://unix.stackexchange.com/questions/3514/how-to-grep-standard-error-stream-stderr)
> the compiler's output to `less`, or into a text file.

Here's my go-to example of how the last error message can mislead you.
Since some of my courses' labs are built around a type named `Book`, I have
seen this situation happen in real life. This specific snippet
of code is a "dramatic reenactment" crafted specifically for this blog post.
([Godbolt.](https://godbolt.org/z/PdfzWzY5r))

    // library.hpp
    namespace library {

      struct book {
        int pagecount_;
        int pagecount() const { return pagecount_; }
      };
      struct ebook {
        int pagecount() const { return 0; }
      };
      bool has_any_pages(const book& b);
      bool has_any_pages(const ebook& b);

    } // namespace library

    // library.cpp
    bool has_any_pages(const ebook& b) {
      return false;
    }

    bool has_any_pages(const book& b) {
      return b.pagecount() > 0;
    }

Clang 15 gives three errors (one with a note) even on such a short snippet.
The third and final error message is produced inside `bool has_any_pages(const book&)`:

    library.cpp:19:13: error: member reference base type
    'const bool' is not a structure or union
        return b.pagecount() > 0;
               ~^~~~~~~~~~

A beginner might be forgiven for thinking that something is wrong with
the return type of `has_any_pages` (although it's unclear why the compiler would
add `const` to that return type), or at least that something is
wrong with the declaration of `book::pagecount`. But if
we'd started with the compiler's _first_ error message, we'd see the problem
right away:

    library.cpp:14:26: error: unknown type name 'ebook';
    did you mean 'library::ebook'?
        bool has_any_pages(const ebook& b) {
                                 ^~~~~
                                 library::ebook

The true problem with our code is that `library.cpp` has forgotten to reopen
`namespace library`, and so name lookup fails to find any `ebook` in
the current scope. In that case Clang intelligently recovers by assuming we must
mean the only `class ebook` in the program, namely `library::ebook`. Each subsequent
error message (if any) will assume that parameter `b` is of type `const library::ebook&`,
which in this case is quite helpful. But look what happens in the next function:

    library.cpp:18:26: error: unknown type name 'book';
    did you mean 'bool'?
        bool has_any_pages(const book& b) {
                                 ^~~~
                                 bool

Here, Clang prefers to "autocorrect" `book` into `bool`, rather than into
`library::book`. So every subsequent error message assumes
that the type of parameter `b` is `const bool&`. When our code asks for
`b.pagecount()`, Clang says, hey, you can't do that: the `const bool` referred
to by parameter `b` isn't a class type and therefore can't be used with the
dot operator. The error message makes sense if you've seen the results of
the "autocorrect" on the previous line; but it's complete nonsense if you haven't.

Therefore, you should always start reading (and fixing) from the _first_
error message. In this case, the first error message was about name lookup on
`ebook`, and was easy to diagnose. As soon as we fix that error — by wrapping
our .cpp file in `namespace library { }` — and recompile, all the other errors
vanish, including the nonsensical third error message.

> Always read the first error message first.

----

See also:

* ["Ways C++ prevents you from doing X"](/blog/2020/03/17/you-cant-do-x/) (2020-03-17)
