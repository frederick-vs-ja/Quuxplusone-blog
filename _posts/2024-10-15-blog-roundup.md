---
layout: post
title: "What I'm reading lately: Greppability, Hebb's dictum"
date: 2024-10-15 00:01:00 +0000
tags:
  blog-roundup
  jokes
  sre
---

A few good blog posts and things (by other people) that I've seen lately:

["How to fork"](https://joaquimrocha.com/2024/09/22/how-to-fork/) (Joaquim Rocha, September 2024)
describes best practices for maintaining a long-term fork of an open-source project
when the upstream is still fairly active. This is exactly the kind of thing I do with
[Clang/libc++](https://github.com/Quuxplusone/llvm-project) and
[VBCC](https://github.com/Quuxplusone/vbccz) — and Joaquim describes exactly how I do it.

----

["Greppability is an underrated code metric"](https://morizbuesing.com/blog/greppability-code-metric/) (Moriz Buesing, August 2024)
articulates one of my pet peeves. Moriz advises "Don't split up identifiers," as in:

    string getTableName(const string& addressType) {
      return addressType + "_addresses";
    }

or:

    assert animal in ["leopard", "tiger", "zebra"]
    filename = animal + ("spot" if animal == "leopard" else "stripe") + ".txt"

Instead, prefer:

    string getTableName(const string& addressType) {
      return (addressType == "shipping") ? "shipping_addresses" :
             (addressType == "billing") ? "billing_addresses" :
             throw OopsError();
    }

    filename = {
      "leopard": "leopardspot.txt",
      "tiger": "tigerstripe.txt",
      "zebra": "zebrastripe.txt",
    }[animal]

That way, someone grepping for "`shipping_addresses`" or "tigerstripe.txt"
has a chance of finding the information they're looking for.

To that I'd add "Don't split up user-visible messages." You have no idea how
annoying it is to find the line of code that produces a message such as
`Error: too many connections to the foo database` when that line is formatted
like:

    my::debug::print_error("too many "
        "connections to the foo "
        "database");

For the sanity of your maintainer (who may be your future self), I recommend
_never_ linebreaking in the middle of any string, with the possible exception
of before or after an escape sequence like `\n` for which you wouldn't expect anybody
ever to grep.

----

UPDATED to add: ["Write code that is easy to delete, not easy to extend"](https://programmingisterrible.com/post/139222674273/write-code-that-is-easy-to-delete-not-easy-to)
(Thomas "tef" Figg, February 2016) is an oldie but a goodie. He deliberately makes
his six headline rules mutually contradictory — "Copy-paste. Don't copy-paste." — but
they remain accurate. He even manages to summarize their appropriate relationships in
the section above the fold.

> Repeat yourself to avoid creating dependencies, but don't repeat yourself to manage them.
> Layer your code: build simple-to-use APIs out of simple-to-implement parts.
> Split your code: isolate the hard-to-write and the likely-to-change parts.
> Don't hard-code every choice, and maybe allow changing a few at runtime.
> Don't try to do all of these things at the same time.
> Maybe don't write so much code in the first place.

----

["Higher-order truths about chmess"](https://personal.lse.ac.uk/robert49/teaching/ph445/notes/dennett.pdf) (Daniel Dennett, 2006)
is a good cold dousing thrown in the direction of some pedantic pursuits.
I certainly resemble its target, sometimes. Dennett writes:

> Some philosophical research projects are
> rather like working out the truths of chess.
> A set of mutually agreed-upon rules are presupposed,
> and the implications of those rules
> are worked out, articulated, debated, refined. So far, so
> good. Chess is a deep and important human artifact,
> about which much of value has been written. But some
> philosophical research projects are more like working
> out the truths of <b>chmess</b>. Chmess is just like chess
> except that the king can move two squares in any
> direction, not one. [...]
> There are just as many a priori truths of chmess as there are
> of chess (an infinity), and they are just as hard to discover.

But (and this is the point) to devote oneself to the deep study of chmess,
to contribute to its scholarly apparatus, is fundamentally a complete
waste of time and energy.

> One good test to make sure you’re not just
> exploring the higher-order truths of chmess is to see if
> people aside from philosophers actually play the
> game. [...]
>
> As Burton Dreben used to say, "Philosophy is garbage,
> but the history of garbage is scholarship." Some garbage
> is more important than other garbage, however, and it’s hard to
> decide which of it is worthy of scholarship.

Dennett quotes Donald Hebb, who said (somewhere, allegedly):

> If it isn't worth doing, it isn't worth doing well.

That is, you shouldn't spend your energy on doing well that which oughtn't to be done at all.
And conversely, perhaps, it should not surprise us when a needless task is also performed poorly.

----

Appropriately for both this Columbus Day weekend and my currently running
book club on Ovid's _Metamorphoses_, I devoured [Richard Armour's](https://en.wikipedia.org/wiki/Richard_Armour)
ersatz history of Europe, [_It All Started with Europa_](https://amzn.to/3BJ6mzz).
Armour (1906–1989) was both a professor and a prolific humorist.
He used to say he had two costumes — cap and gown and cap and bells.

> "If a thing is worth doing at all," said Ivan IV, "it is worth doing terribly."
>
> Catherine the Great frequently entertained literary personages such as Diderot,
> who complimented her on her mind, a part of her which had been overlooked
> by most of her friends.
>
> Some say that Catherine corresponded with Voltaire,
> but actually the similarity was slight.
