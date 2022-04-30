---
layout: post
title: 'To enable ADL, add a using-declaration'
date: 2022-04-30 00:01:00 +0000
tags:
  argument-dependent-lookup
  c++-style
  customization-points
  library-design
  name-lookup
  paradigm-shift
---

This is yet another followup to ["What is the `std::swap` two-step?"](/blog/2020/07/11/the-std-swap-two-step/) (2020-07-11)
and ["PSA: ADL requires that unqualified lookup has found a function"](/blog/2022/04/14/another-reason-for-the-poison-pill/) (2022-04-14),
as my mental model continues to evolve. (Mainly due to pressure from Jody Hagins. :))

Consider this reduced version of the `plf::hive::iterator::distance` function, as I actually
encountered it in the wild last week. ([Godbolt.](https://godbolt.org/z/abYP89YEa))

    struct ctr {
        void swap(ctr&) noexcept;
        friend void swap(ctr& a, ctr& b) noexcept { a.swap(b); }

        struct iterator;
    };

    struct ctr::iterator {
        int m_;

        friend auto operator<=>(iterator, iterator) = default;

        friend void swap(iterator& a, iterator& b) noexcept {
            std::swap(a.m_, b.m_); // A
        }

        int distance(iterator last) {
            iterator first = *this;
            bool should_swap = (first > last);
            if (should_swap) {
                swap(first, last); // B
            }
            return 42;
        }
    };

On the line marked `B`, we get a compiler error: GCC says

    error: no matching function for call to 'ctr::swap(ctr::iterator&, ctr::iterator&)'
       25 |             swap(first, last);
          |             ~~~~^~~~~~~~~~~~~
    note: candidate: 'void ctr::swap(ctr&)'
        5 |     void swap(ctr&) noexcept;
          |          ^~~~

while Clang says

    error: call to non-static member function 'swap' of 'ctr' from nested type 'iterator'
                swap(first, last);
                ^~~~

Of course we _expected_ our call to `swap(first, last)` to find the `swap` that is a
friend of `ctr::iterator` (line `A`). We're in the context of `ctr::iterator`, and that's
`ctr::iterator`'s `swap` function — why wouldn't it be found?

But in fact hidden friends are found only by ADL, and _ADL does not happen on line `B`._
ADL is always conditional ([[basic.lookup.argdep]/1](https://eel.is/c++draft/basic.lookup.argdep#1)):
it happens only if the initial unqualified lookup of `f` finds a non-block-scope, non-member
function declaration. And in this specific case, an unqualified lookup of the identifier `swap`
in the scope of `ctr::iterator::distance` actually finds... member function `ctr::swap`!
Because `ctr::swap` is a member function, ADL is not performed.

The way to fix this code is to add a `using`-declaration:

        if (should_swap) {
            using std::swap;
            swap(first, last); // B, now OK
        }

My previous mental model would be utterly confused by this construction. Notice that
there is no situation in which line `B` will ever actually _call_ `std::swap`; line `B`
always, invariably, calls the `swap` which is a friend of `ctr::iterator`, a specific
concrete class type. This code features no templates at all; it is completely non-generic.
So why are we mentioning `std::swap`, a function we'll never call?

----

In my previous post (["PSA: ADL requires that unqualified lookup has found a function"](/blog/2022/04/14/another-reason-for-the-poison-pill/) (2022-04-14))
I wrote:

> For this blog post's purposes, the third and most important effect of a "poison pill"
> declaration is to make sure that unqualified lookup finds a
> function declaration — instead of searching all the way out to the
> global scope where an evil user might have declared a non-function with that
> name, thus preventing ADL.
>
> Library implementors take note: bare unqualified ADL is probably a bad idea!
> If you're not using the `std::swap` two-step, then use a "poison pill" declaration
> to ensure that your unqualified lookup never finds a non-function.

Jody Hagins tells me he's always phrased it stronger, and I'm coming to agree with him:
I'm now more than 50% certain that your mental model should be

> A `using`-declaration for a function, like `using std::swap;`, <b>enables ADL</b> for that name.
> If you intend a specific call-site to use ADL, you must add a `using`-declaration
> in the scope of that call-site. That `using`-declaration might still name a "fallback"
> to use when ADL fails, but the <b>primary</b> effect of such a declaration is
> always <b>to enable ADL.</b>

Conversely, if you are reading through a codebase and you see a `using`-declaration,
that should be a pretty good sign that the original author intended ADL to happen on that
name, somewhere in that scope.
