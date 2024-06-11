---
layout: post
title: "Tangent circles of integer radius"
date: 2024-06-10 00:01:00 +0000
tags:
  help-wanted
  math
  pretty-pictures
  puzzles
excerpt: |
  Over on [Puzzling StackExchange](https://puzzling.stackexchange.com/questions/126548/geometry-puzzle-tangent-circles-with-integer-radii),
  Brandan Williams poses the following interesting question:

  > Find a strictly decreasing sequence of integers $$r_0, r_1, r_2, \dots, r_n$$ such that
  > you can place kissing circles of radii $$r_1, r_2, \dots, r_n$$
  > around a central circle of radius $$r_0$$. That is, circle $$r_0$$
  > will be tangent to all $$n$$ other circles, and circle $$r_i$$ will be
  > tangent to circle $$r_{i+1}$$ for all $$1\leq i<n$$, and circle $$r_n$$ will be tangent to circle $$r_1$$.
---

Over on [Puzzling StackExchange](https://puzzling.stackexchange.com/questions/126548/geometry-puzzle-tangent-circles-with-integer-radii),
Brandan Williams poses the following interesting question:

> Find a strictly decreasing sequence of integers $$r_0, r_1, r_2, \dots, r_n$$ such that
> you can place kissing circles of radii $$r_1, r_2, \dots, r_n$$
> around a central circle of radius $$r_0$$. That is, circle $$r_0$$
> will be tangent to all $$n$$ other circles, and circle $$r_i$$ will be
> tangent to circle $$r_{i+1}$$ for all $$1\leq i<n$$, and circle $$r_n$$ will be tangent to circle $$r_1$$.

If it weren't for the "strictly decreasing" criterion, a couple of simple solutions would be
(1 1 1 1 1 1 1) and (1 3 2 3 2).

<table>
  <tr>
    <td><img width="100%" style="vertical-align: middle;" src="/blog/images/2024-06-10-circles-seven-1s.svg"/></td>
    <td><img width="100%" style="vertical-align: middle;" src="/blog/images/2024-06-10-circles-1-3-2-3-2.svg"/></td>
  </tr>
</table>

A more complicated example (still failing the "decreasing" criterion) would be this smallest integer solution
to the [four coins problem](https://mathworld.wolfram.com/FourCoinsProblem.html), (6 23 46 69):

![](/blog/images/2024-06-10-circles-6-23-46-69.svg)

> If I'm not mistaken that this is the smallest distinct-integer-valued solution
> to the four coins problem, then I'm surprised it's not better known.
> The only place I find the sequence "6, 23, 46, 69" recorded online is in
> [this set of math puzzles](https://sumo.stanford.edu/pdfs/smt2020/geometry-problems.pdf)
> from Stanford in 2020; see the diagram in problem 10. The radius-6 circle is the
> [inner Soddy circle](https://en.wikipedia.org/wiki/Soddy_circles_of_a_triangle) of the other three;
> their outer Soddy circle circumscribes the outer circles and (if I'm not mistaken) would have radius 138.

StackExchange contributor "EdwardH" wrote up [a really fantastic procedure](https://puzzling.stackexchange.com/questions/126548/geometry-puzzle-tangent-circles-with-integer-radii/126674#126674)
to find solutions that *do* satisfy the "strictly decreasing" criterion. I turned it into Python code
([here](https://github.com/Quuxplusone/RecreationalMath/tree/master/TangentCircles); the code that generated
these pretty SVG images is also there) and let my laptop loose on it for a while; but after multiple days
of mainly brute force, my best solution remained superastronomically large:

![](/blog/images/2024-06-10-circles-2941564115506288009572255050208.svg)

Tom Sirgedas discovered that it works better to look for solutions with an odd number of circles (i.e.,
an even number of circles around the central one). Here's his record as of this writing. These radii are
small enough to fit in a 32-bit `int`!

![](/blog/images/2024-06-10-circles-476991963.svg)

Now, the obvious question is: Is Tom's the *smallest* solution to this puzzle (measured by the radius of
the central circle)? If not, then what is the smallest solution? And how do you prove it? Help wanted!

----

See also these two posts, for the musings that led up to the invention of this problem (albeit no further
investigation of the problem itself):

* ["Convergent Chain Curling"](https://achromath.substack.com/p/convergent-chain-curling) (Brandan Williams, July 2022)
* ["Convergent Chain Curling Part 2"](https://achromath.substack.com/p/convergent-chain-curling-part-2) (Brandan Williams, April 2024)
