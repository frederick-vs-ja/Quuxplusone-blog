---
layout: post
title: 'My C++Now 2019 talks are captioned'
date: 2019-06-25 00:01:00 +0000
tags:
  conferences
  cppnow
  relocatability
---

The sessions I presented at C++Now 2019 are now up on YouTube, and fully captioned
in English by me!

----

My one main-program session this year was a 90-minute summary of ["Trivially Relocatable"](https://www.youtube.com/watch?v=SGdfPextuAU).
Regular readers of this blog's [#relocatability tag](/blog/tags/#relocatability)
will have seen a lot of the raw material that went into this talk; but (A) there's some new material too, and
more importantly (B) in this talk I pull together all of the material into one place, organized. "Part 1" of the
talk is where I try to hook newcomers on the performance and safety benefits of P1144 `[[trivially_relocatable]]`.
"Part 2" — the more important part for non-newcomers to the subject — is where I compare P1144 against each
prior proposal in the area, including
[N4158 destructive move](/blog/2018/09/28/trivially-relocatable-vs-destructive-movable/)
and [`[[clang::trivial_abi]]`](/blog/2018/05/02/trivial-abi-101/), but also plenty of things that I haven't blogged
about yet, such as Niall Douglas's [P1029R1](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1029r1.pdf)
(which got an R2 facelift in the pre-Cologne WG21 mailing but still has all the same problems) and the problem of
shared-memory "persistent" data structures.

I also gave a comic five-minute lightning talk titled ["Boost.Blockchain"](https://www.youtube.com/watch?v=2v2N12xeruc).
I'd been meaning to give a talk with that premise for a few years now, but this is the year
it finally clicked. C++2a certainly provides no shortage of soft targets for satire.

I am proud of my two talks. :)

> In case you're wondering: Yes, I made myself a written script for that lightning talk. Yes, I practiced beforehand.
> No, neither of my practice runs actually finished in under 6 minutes! So I was pleasantly surprised that I managed,
> in the event, to come in right at the allotted 5 minutes.

----

My captions include the audience's comments transcribed (and attributed,
where I was sure of the attribution).  And with footnotes!  I tried to provide a link
every time I alluded to another C++Now session or to one of my blog posts.

I am proud of my captions.

If you are fluent in a language other than English, I invite and encourage you to download the .sbv file,
[translate it into your own native language, and upload your translation!](http://cppvap.wikidot.com/wiki:captions-catalog)
