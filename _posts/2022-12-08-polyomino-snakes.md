---
layout: post
title: "Polyomino strips, snakes, and ouroboroi"
date: 2022-12-08 00:01:00 +0000
tags:
  math
  pretty-pictures
  puzzles
excerpt: |
  Previously on this blog: ["Polycube snakes and ouroboroi"](/blog/2022/11/18/polycube-snakes/) (2022-11-18).

  Preparing to add the sequences from that post into [OEIS](https://oeis.org), I realized
  to my surprise that most of the 2D analogues weren't yet in OEIS either! There are several
  near-miss variations, though, involving the following concepts:
---

Previously on this blog: ["Polycube snakes and ouroboroi"](/blog/2022/11/18/polycube-snakes/) (2022-11-18).

Preparing to add the sequences from that post into [OEIS](https://oeis.org), I realized
to my surprise that most of the 2D analogues weren't yet in OEIS either! There are several
near-miss variations, though, involving the following concepts:


## Polyominoes with holes (snakes versus strips)

Any polyomino divides the plane into one or more rookwise-connected regions. Two chess rooks
are in different regions if one cannot reach the other in any number of rookwise moves without
at some point passing through a cell of the polyomino. A polyomino which divides the plane into
$$k+1$$ regions is said to have $$k$$ holes.

The smallest polyomino with a hole is the O-heptomino... which also happens to be a polyomino snake.

![Two rooks divided by a snake](/blog/images/2022-12-08-o-heptomino.svg)

The polyomino divides the two pictured rooks from each other; therefore it is _not_ a strip.

[Miroslav Vicher](http://www.vicher.cz/puzzle/polyform/minio/polystrip.htm)
refers to polyomino snakes without holes as _strip polyominoes_. Strip polyominoes are interesting
if your main recreational interests in polyominoes are tilings and dissections, because holes pose
a problem for those tasks. The 30 free heptomino-strips can tile a pretty 14x15 rectangle:

![Image credit: Miroslav Vicher](/blog/images/2022-12-08-vicher-heptastrips.png)

but the 31 heptomino-snakes can't tile any hole-free shape at all.
Vicher notes that the 150 free
[nonomino](https://en.wikipedia.org/wiki/Nonomino)-strips cannot tile any
rectangle, either, because the nonomino depicted below has — not a _hole_, but
what Vicher calls a _cave_.

![Image credit: Miroslav Vicher](/blog/images/2022-12-08-vicher-nonomino-with-cave.png)

Although the interior of the cave can be reached
by a freely moving rook, it cannot be filled by any possible combination of snakes.

> Vicher's website often uses the shorthand "polystrip" to mean "strip polyomino";
> but that's unnecessarily confusing, given the established convention of using the
> suffix to identify the base form: polyomino, polyiamond, polyabolo, polyhex.
> We can speak also of strip polyiamonds, strip polyabolos, strip polyhexes;
> and snakes and ouroboroi of those forms as well.

OEIS sequences demonstrating the effect of holes include:

* [OEIS A049429](https://oeis.org/A049429) counts free _d_-dimensional polycubes
  * Summing across the first two columns of each row gives [A000105](https://oeis.org/A000105), the count of free 2D polyominoes
  * Summing across the first three columns of each row gives [A038119](https://oeis.org/A038119), the count of free 3D polycubes
  * Summing across the first four columns of each row gives [A068870](https://oeis.org/A068870), the count of free 4D polyhypercubes


* [OEIS A000105](https://oeis.org/A000105) counts free polyominoes
  * [OEIS A000104](https://oeis.org/A000104) counts free polyominoes with no holes
  * [OEIS A057418](https://oeis.org/A057418) counts free polyominoes with exactly one hole
  * [OEIS A001419](https://oeis.org/A001419) counts free polyominoes with one or more holes (i.e., [A000105](https://oeis.org/A000105) minus [A000104](https://oeis.org/A000104))


* [OEIS A002013](https://oeis.org/A002013) counts free snake polyominoes
  * [OEIS A333313](https://oeis.org/A333313) counts free snake polyominoes with no holes (i.e., strip polyominoes)


* [OEIS A038119](https://oeis.org/A038119) counts free polycubes
  * [OEIS A357083](https://oeis.org/A357083) counts free polycubes with one or more holes

Just to confuse matters, the title of [OEIS A151514](https://oeis.org/A151514) and
a 2009 comment on [OEIS A002013](https://oeis.org/A002013) currently use the term "strip"
when they actually mean "snake." I have submitted corrections.


### Holes in 3D

There are at least two ways to define the 3D equivalent of a "strip" (resp. "a snake without holes").

* The simplest kind of 3D "hole" is a fully enclosed empty region from which a 3D rook could not escape
    by 3D-rookwise moves; this is also known as a "cavity." The smallest polycube with such a cavity is
    also a snake: `SRDRURDRUR` wraps around all six faces of an empty cell, in the same way as the
    O-heptomino wraps around all four sides of an empty 2D cell.
    Similarly, the shortest polyhypercube snake with a cavity would use 15 hypercubes
    to wrap around all eight hyperfaces of an empty 4D cell; and so on.

* A different kind of 3D hole is the "donut hole": a tunnel through which you can pass a string,
    then connect the ends of the string to form a loop, such that the loop of string cannot escape
    to infinity no matter how much it wiggles. Of the three free eight-cube ouroboroi, one (the
    simplest) has a donut hole, and the other two don't. Of the two chiral ten-cube ouroboroi, one
    has a donut hole, and the other (being the polycube equivalent of a [horn torus](https://mathworld.wolfram.com/HornTorus.html))
    doesn't.

* My lovely wife pointed out the donut-hole possibility; I then further observed that some polycube ouroboroi
    have what I might call "pseudo-tunnels," where the ouroboros itself actually does not circumnavigate
    the tunnel, but simply doubles back on itself. We can characterize these pseudo-tunnels by whether
    our loop of string can escape them by wiggling through _corners_, or whether it must wiggle through
    _edges_ as well. I believe the smallest polycube ouroboros with an "edge-type pseudo-tunnel" is this
    quite appropriately shaped 22-cube construction:

![Sam's heart-shaped ouroboros](/blog/images/2022-12-08-sams-heart-polycube.jpg)

A minor tweak gives a 24-cube ouroboros with a "corner-type pseudo-tunnel"; this can be reduced
to a 22-cube ouroboros (see [my followup post](/blog/2022/12/11/lego-polycube-snakes/)).


## One-sided versus free polyominoes

Whenever we talk about the distinctness of _d_-dimensional shapes, we must clarify what kinds
of rotations we're permitting.

* No rotations at all: "fixed" polyominoes
* Rotations in _d_-space but no higher: "one-sided" polyominoes
* Rotations through _d+1_-space: "two-sided" or "free" polyominoes

When counting "one-sided" 2D polyominoes, we don't allow flipping them over through 3-space: mirror images
are counted as distinct. Likewise, when counting "one-sided" 3D polycubes, we don't allow flipping them
through hyperspace: mirror images are counted as distinct. A polyform with distinct left- and right-handed
variants is called "chiral": its left- and right-handed variants are counted as two distinct one-sided polyforms,
but identified as the same free polyform.

This notion of one-sidedness is different from my previous blog post's notion
of "directed snakes," where I was distinguishing the snake's head from its tail.

> The mathematical term for that concept seems to be "rootedness," although you might need
> some more verbiage to explain that a "rooted snake" must have its root actually at one of
> its ends, and not somewhere in the middle.

The following image shows just four free undirected polyomino snakes (the I, L, W, and Z pentominoes);
but since the L and Z are chiral, we have a total of _six_ one-sided undirected snakes in the left half
of the image. Meanwhile, variants of the L and W produce _six_ free directed/rooted snakes
across the top half of the image; and the entire image shows _nine_ examples of one-sided directed snakes.
(Of course this is just a representative sampling of the _total_ number of one-sided directed snake pentominoes.)

![Chiral and directed snake pentominoes](/blog/images/2022-12-08-chiral-pentominoes.svg)

OEIS sequences demonstrating the effect of chirality include:

* [OEIS A000988](https://oeis.org/A000988) counts one-sided polyominoes, which is the sum of:
  * [OEIS A000105](https://oeis.org/A000105) counts free polyominoes
  * [OEIS A030228](https://oeis.org/A030228) counts chiral polyominoes (i.e., [A000988](https://oeis.org/A000988) minus [A000105](https://oeis.org/A000105))

* [OEIS A000162](https://oeis.org/A000162) counts one-sided polycubes ([A355966](https://oeis.org/A355966) counts those with cavities)
  * [OEIS A038119](https://oeis.org/A038119) counts free polycubes ([A357083](https://oeis.org/A357083) counts those with cavities)

* [OEIS A068870](https://oeis.org/A068870) counts one-sided polyhypercubes
  * [OEIS A255487](https://oeis.org/A255487) counts free polyhypercubes

* [OEIS A151514](https://oeis.org/A151514) counts one-sided snake polyominoes
  * [OEIS A002013](https://oeis.org/A002013) counts free snake polyominoes

* An unlisted sequence counts one-sided strip polyominoes
  * [OEIS A333313](https://oeis.org/A333313) counts free strip polyominoes


## Table of results (2D)

Here are my results for the 2D polyomino cases ([source code](/blog/code/2022-12-08-polyomino-snakes-and-strips.cpp)).
Columns corresponding to existing OEIS sequences are marked accordingly.

| n  | Free strip polyominoes ([A333313](https://oeis.org/A333313)) | Free non-ouroboros snakes ([A002013](https://oeis.org/A002013)) | Free ouroboroi | One-sided strips | One-sided non-ouroboros snakes ([A151514](https://oeis.org/A151514)) | One-sided ouroboroi |
|----|------------|------------|--------|------------|------------|--------|
| 1  | 1          | 1          |        | 1          | 1          |        |
| 2  | 1          | 1          |        | 1          | 1          |        |
| 3  | 2          | 2          |        | 2          | 2          |        |
| 4  | 3          | 3          | 1      | 5          | 5          | 1      |
| 5  | 7          | 7          |        | 10         | 10         |        |
| 6  | 13         | 13         | 0      | 24         | 24         | 0      |
| 7  | 30         | 31         |        | 52         | 53         |        |
| 8  | 64         | 65         | 1      | 124        | 126        | 1      |
| 9  | 150        | 154        |        | 282        | 289        |        |
| 10 | 338        | 347        | 1      | 668        | 686        | 1      |
| 11 | 794        | 824        |        | 1548       | 1604       |        |
| 12 | 1836       | 1905       | 4      | 3654       | 3792       | 4      |
| 13 | 4313       | 4512       |        | 8533       | 8925       |        |
| 14 | 10067      | 10546      | 7      | 20093      | 21051      | 11     |
| 15 | 23621      | 24935      |        | 47033      | 49638      |        |
| 16 | 55313      | 58476      | 31     | 110533     | 116858     | 45     |
| 17 | 129647     | 138002     |        | 258807     | 275480     |        |
| 18 | 303720     | 323894     | 95     | 607227     | 647573     | 178    |
| 19 | 711078     | 763172     |        | 1421055    | 1525113    |        |
| 20 | 1665037    | 1790585    | 420    | 3329585    | 3580673    | 762    |
| 21 | 3894282    | 4213061    |        | 7785995    | 8423334    |        |
| 22 | 9111343    | 9878541    | 1682   | 18221563   | 19755938   | 3309   |
| 23 | 21290577   | 23214728   |        | 42575336   | 46422915   |        |
| 24 | 49770844   | 54393063   | 7544   | 99539106   | 108783480  | 14725  |
| 25 | 116206114  | 127687369  |        | 232398659  | 255359883  |        |
| 26 | 271435025  | 298969219  | 33288  | 542864111  | 597932342  | 66323  |
| 27 | 633298969  | 701171557  |        | 1266567155 | 1402308318 |        |
| 28 | 1478178004 | 1640683309 | 152022 | 2956342341 | 3281352516 | 302342 |
| 29 | 3446626028 | 3844724417 |        | 6893180336 | 7689369625 |        |
{:.smaller}

## Table of results (3D)

Here's a partial table of 3D polycube cases.
Columns corresponding to existing OEIS sequences are marked accordingly;
which is to say, none of these sequences seem to be in the OEIS quite yet.
Blanks (except in the odd ouroboros cases) indicate that I just haven't
computed those values yet; but I'm working on filling those in
([source code](/blog/code/2022-12-08-polycube-snakes-and-strips.cpp)).

The table below indicates the existence of two _chiral pairs_ among the 10-cube ouroboroi.
Can you find them? How about the three different ways to enclose a cavity with a 12-cube
ouroboros (one mirror-symmetric, the other two forming a chiral pair)?

| n  | Free non-ouroboros snakes | Free non-ouroboros snakes with cavities | Free ouroboroi | Free ouroboroi with cavities | One-sided non-ouroboros snakes | One-sided non-ouroboros snakes with cavities | One-sided ouroboroi | One-sided ouroboroi with cavities |
| 1  | 1          |            |        |        | 1             |               |          |          |
| 2  | 1          |            |        |        | 1             |               |          |          |
| 3  | 2          |            |        |        | 2             |               |          |          |
| 4  | 4          |            | 1      |        | 5             |               | 1        |          |
| 5  | 12         |            |        |        | 16            |               |          |          |
| 6  | 34         |            | 1      |        | 54            |               | 1        |          |
| 7  | 125        |            |        |        | 212           |               |          |          |
| 8  | 450        |            | 3      |        | 827           |               | 3        |          |
| 9  | 1780       |            |        |        | 3369          |               |          |          |
| 10 | 7021       |            | 11     |        | 13653         |               | 13       |          |
| 11 | 28521      | 4          |        |        | 56052         | 8             |          |          |
| 12 | 115553     | 5          | 77     | 2      | 229004        | 10            | 122      | 3        |
| 13 | 472578     | 24         |        |        | 939935        | 48            |          |          |
| 14 | 1927634    | 105        | 606    | 0      | 3843859       | 210           | 1115     | 0        |
| 15 | 7890893    | 485        |        |        | 15753903      | 970           |          |          |
| 16 | 32221475   | 2098       | 6465   | 4      | 64380796      | 4196          | 12562    | 8        |
| 17 | 131812746  | 9381       |        |        | 263475472     | 18762         |          |          |
| 18 | 538059836  | 40566      | 74314  | 23     | 1075780425    | 81132         | 147350   | 46       |
| 19 |            |            |        |        | 4397161320    |               |          |          |
| 20 |            |            |        |        | 17939394036   |               | 1810165  |          |
| 21 |            |            |        |        | 73251877235   |               |          |          |
| 22 |            |            |        |        | 298646347226  |               | 22812552 |          |
| 23 |            |            |        |        | 1218453344740 |               |          |          |
{:.smaller}

----

See also:

* ["Lego polycube snakes"](/blog/2022/12/11/lego-polycube-snakes/) (2022-12-11)
