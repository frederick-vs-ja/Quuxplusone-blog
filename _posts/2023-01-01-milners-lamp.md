---
layout: post
title: "Milner's lamp"
date: 2023-01-01 00:01:00 +0000
tags:
  math
  old-shit
  puzzles
excerpt: |
  Quoted in ["Problems for Solution,"](https://www.jstor.org/stable/2972299?seq=2)
  _American Mathematical Monthly_ <b>28</b>:4 (April 1921):

  > In the [_Journal of the Indian Mathematical Society_, June 1920](https://babel.hathitrust.org/cgi/pt?id=iau.31858027819071&view=1up&seq=393)
  > [...] A. Narasinga Rao
  > [proposes]: "Determine generally the form of a vessel whose contents are just spilling over
  > in the position of equilibrium, whatever the amount of liquid it contains, (1) when it
  > rests on a horizontal plane; (2) when it is suspended about a horizontal axis."

  This problem is known as "Dr. Milner's lamp," after [Isaac Milner](https://en.wikipedia.org/wiki/Isaac_Milner) (1750–1820),
  Dean of Queen's College, Cambridge. Milner's having designed an idiosyncratic lamp is mentioned in
  [the biography written posthumously by his niece](https://books.google.com/books?id=5gFjAAAAcAAJ&pg=PA365),
  although she doesn't imply anything mathematically interesting or ideal about the lamp's shape, merely
  that it was a well-crafted reading lamp popular among Milner's colleagues.
  (As a Fellow of the Royal Society, Milner corresponded with Sir Humphry Davy (1778–1829),
  inventor of [a useful lamp of his own](https://en.wikipedia.org/wiki/Davy_lamp).)
---

Quoted in ["Problems for Solution,"](https://www.jstor.org/stable/2972299?seq=2)
_American Mathematical Monthly_ <b>28</b>:4 (April 1921):

> In the [_Journal of the Indian Mathematical Society_, June 1920](https://babel.hathitrust.org/cgi/pt?id=iau.31858027819071&view=1up&seq=393)
> [...] A. Narasinga Rao
> [proposes]: "Determine generally the form of a vessel whose contents are just spilling over
> in the position of equilibrium, whatever the amount of liquid it contains, (1) when it
> rests on a horizontal plane; (2) when it is suspended about a horizontal axis."

This problem is known as "Dr. Milner's lamp," after [Isaac Milner](https://en.wikipedia.org/wiki/Isaac_Milner) (1750–1820),
Dean of Queen's College, Cambridge. Milner's having designed an idiosyncratic lamp is mentioned in
[the biography written posthumously by his niece](https://books.google.com/books?id=5gFjAAAAcAAJ&pg=PA365),
although she doesn't imply anything mathematically interesting or ideal about the lamp's shape, merely
that it was a well-crafted reading lamp popular among Milner's colleagues.
(As a Fellow of the Royal Society, Milner corresponded with Sir Humphry Davy (1778–1829),
inventor of [a useful lamp of his own](https://en.wikipedia.org/wiki/Davy_lamp).)

Augustus De Morgan in [_A Budget of Paradoxes_](https://books.google.com/books?id=kfPuAAAAMAAJ&pg=PA252)
describes the apocryphal lamp in terms of Rao's formulation (2):

> A hollow semi-cylinder [...] revolved on pivots. The curve was calculated [such] that,
> whatever quantity of oil might be in the lamp, the position of equilibrium just brought
> the oil up to the edge of the cylinder, at which a bit of wick was placed. As the wick exhausted
> the oil, the cylinder slowly revolved about the pivots so as to keep the oil always touching
> the wick.

Arthur Cayley's ["On a Differential Equation and the Construction of Milner's Lamp,"](https://rcin.org.pl/impan/Content/143709/WA35_176624_12807-13_Art889.pdf)
_Proceedings of the Edinburgh Mathematical Society_ <b>5</b> (1887), gives a solution of problem (1).
I'm unclear on Cayley's assumptions/invariants, but the result seems to be practical. It starts with
a semicircular cylinder, then adds a weight in the shape of a [segment](https://mathworld.wolfram.com/CircularSegment.html)
having the same density as the oil, and coming right up to the lip. This ensures that as the level of
the oil is infinitesimally lowered, the vessel tips infinitesimally toward the lip; yet the center
of gravity of the whole weight-plus-oil system always remains centered over the vessel's point of contact
with the table.

![The illustration from Cayley's paper](/blog/images/2023-01-01-cayleys-solution.png)

The same approach would work for a spherical vessel, circular hole, and [cap](https://mathworld.wolfram.com/SphericalCap.html)-shaped
weight.

Now, if the weight is heavier than the oil, the whole thing falls apart. (Imagine the limit, where
the weight has density 1 and the oil has density close to 0.) But you can replace the
weight with a slight inward-pointing "lip," which accomplishes the same goal as Cayley's weight.

![The oil above the lip burns (middle); the lamp tips left (right).](/blog/images/2023-01-01-lip-lamp.svg)

Vice versa, it seems to me that the whole solution falls apart if we consider the density of the wall of
the vessel itself. Again in the limit, if the semicircular wall of the container has density 1 and the oil
has density close to 0, then the equilibrium position will simply have the opening at the top:

![Not a finger!](/blog/images/2023-01-01-busted-lip-lamp.svg)

Now, Cayley's solution might be "cheating," as far as the original problem statement is concerned,
because it involves a solid weight besides the oil; or, equivalently, a container of non-uniform density.

The _American Mathematical Monthly_ credits "D. Biddle" with a solution to (2), published in
[_Mathematical Questions and Solutions from the "Educational Times"_](https://books.google.com/books?id=088GAAAAYAAJ&pg=PA54)
<b>49</b> (1888), pp. 54-55. At first, Biddle seems to me to be proving that the ideal solution (again assuming weightless walls)
must be a circular arc. Assuming that the bowl pivots about $$P$$, then:

> Let $$AC_1$$, $$AC_2$$, $$AC_3$$ represent the surface of the oil at three different times;
> then in $$PB_1$$, $$PB_2$$, $$PB_3$$ perpendicular to these, respectively, will lie the centre of gravity
> of the oil at those particular times. Consequently the level of the oil, always flush with A,
> and the line joining the centre of gravity with P, describe equal angles in a given time.

However, from this Biddle somehow derives that an appropriate curve would be $$r=\sqrt{\cos\theta}$$
(see the graph at [Wolfram Alpha](https://www.wolframalpha.com/input?i=r=sqrt(cos(theta-pi/8))+for+7pi/8+to+19pi/8)),
with the pivot $$P$$ located somewhere difficult-to-compute in the middle. (All you'd have to do is plot
any two center-of-gravity lines $$PB_i$$ and take their intersection; but those lines are themselves
difficult to compute.)

No particular conclusions here — my math isn't up to the task! But it would be interesting

- to see a Cayley-style vessel constructed in real life;

- to see a solution that doesn't assume the vessel's walls are weightless;

- to see a proof that Rao's formulations (1) and (2) are equivalent for weightless vessels and
    non-equivalent for vessels with weight;

- to find out if Isaac Milner's actual historical lamp resembled this mathematical recreation
    in any way, or if the situation is completely apocryphal.

----

Incidentally, the specific June 1920 issue of the _Journal of the Indian Mathematical Society_ mentioned
above contains [many touching eulogies](https://babel.hathitrust.org/cgi/pt?id=iau.31858027819071&view=1up&seq=355)
of [Srinivasa Ramanujan](https://en.wikipedia.org/wiki/Srinivasa_Ramanujan), who had died in April of that year.
A. Narasinga Rao (third-from-left in the top row of
[this photo](https://archive.org/details/dli.csl.6252/page/n111/mode/1up) of the
staff of Presidency College, Madras, 1925) went on to become the first editor of the Indian Mathematical Society's
[_Mathematics Student_](https://www.indianmathsociety.org.in/ms.htm).
