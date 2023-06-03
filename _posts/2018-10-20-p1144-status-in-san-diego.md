---
layout: post
title: "I won't be in San Diego"
date: 2018-10-20 00:01:00 +0000
tags:
  relocatability
  san-diego-2018
---

People keep asking me if I'll be at the WG21 meeting in San Diego this November.
No, I won't; I have a prior commitment elsewhere.

(The quietness of my blog is unrelated; I'm currently on a cross-country vacation
with my wonderful wife.)

I have submitted four papers for San Diego. I present them here in order of significance:

- [P1153 "Copying volatile subobjects is non-trivial."](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1153r0.html)
This one is a conservative attempt to solve the problem identified in my blog post
["When is a trivially copyable object not trivially
copyable?"](/blog/2018/07/13/trivially-copyable-corner-cases/) (2018-07-13).
It has been coauthored and will be shepherded at San Diego by JF Bastien, along with his own
[P1152 "Deprecating `volatile`"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1152r0.html)
at the same time. I suspect that P1152 will perform better than P1153 for social reasons,
and therefore P1153 is probably negligible. I'm okay with this.

- [P1154 "Type traits for structural comparison."](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1154r0.html)
This paper proposes that there should be a standard way to detect the property of
strong structural equality, so that we can for example SFINAE or `static_assert` on that property.
(Strong structural equality is the property brought into the C++2a Working Draft by Jeff Snyder and
Louis Dionne's [P0732 "Class Types in Non-Type Template Parameters."](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0732r2.pdf))
Jeff Snyder is a coauthor and shepherd on this paper. I expect and hope that under Jeff's wing
P1154 will sail through LEWG and LWG, being as it is a friendly amendment to the already-accepted P0732.

- [P1155 "More implicit moves."](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1155r0.html)
This was the subject of my CppCon talk ["RVO is Harder Than it Looks: The Story of
`-Wreturn-std-move`."](https://cppcon2018.sched.com/event/FnL2/rvo-is-harder-than-it-looks-the-story-of-wreturn-std-move)
It proposes that we should make `return x;` into a move-from-`x` in most cases, so that maybe in a few years
I won't just have to _update_ that talk but maybe I'll no longer have to give it at all!
David Stone is a coauthor and shepherd on this paper, along with his own related but orthogonal
[P0527R1 "Implicitly move from rvalue references in return statements."](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0527r1.html)
I am uncertain whether P0527 is a slam-dunk, although it has nice ramifications as discussed
[here](/blog/2018/09/25/perfect-backwarding/#but-it-occurs-to-me-that-we-can).
My tentative hope is that we can get both P1155 _and_ P0527 for C++2a. P1155 includes proposed wording
relative to the current working draft and also relative to the-working-draft-with-P0527-applied.

- [P1144 "Object relocation in terms of move plus destroy."](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1144r0.html)
This, the most important of my papers by far, has seen a lot of
anecdotal interest, but didn't even have a shepherd until the other day, when Corentin Jabot stepped up
in alarm after seeing that P1144 hadn't been scheduled for discussion in any group!
It looks like now it's going to be seen by the newly formed "EWGI" ("Evolution Working Group Incubator")
at some point during San Diego week. It is essentially unbelievable at this point that P1144 trivial
relocatability would make it into C++2a.

If you'd like to see P1144 in C++2a, and you'll be at San Diego, please try to be in
the room when it's discussed! Many thanks for Corentin Jabot for becoming sufficiently alarmed to
try to get it discussed — I hope it happens. :)

If you'd like to see P1144 in Clang, and you are a reviewer for Clang/LLVM, please leave your review comments on
[the Clang pull request](https://reviews.llvm.org/D50119) (open since 2018-07-31).  Many thanks to Nicolas Lesser
for his intensive commenting so far.

For more information on P1144 trivial relocatability, see the
[posts tagged #relocatability](/blog/tags/#relocatability) on this blog.
