---
layout: post
title: "Pentoprimality"
date: 2024-01-15 00:01:00 +0000
tags:
  math
  pretty-pictures
  puzzles
excerpt: |
  Over on [/r/askmath](https://old.reddit.com/r/askmath/comments/194jmfe/pentoprimality_challenge_there_are_2339_unique/)
  a couple days ago, "electricmaster23" posed the following puzzle:

  > Pack the 12 pentominoes into a 6x10 rectangle, then label each cell
  > of that rectangle with a digit from 1 to 5 such that each pentomino contains
  > all of 1 through 5, and, whenever two adjacent cells belong to different
  > pentominoes, their labels' sum is a prime number.
---

Over on [/r/askmath](https://old.reddit.com/r/askmath/comments/194jmfe/pentoprimality_challenge_there_are_2339_unique/)
a couple days ago, "electricmaster23" posed the following puzzle:

> Pack the 12 pentominoes into a 6x10 rectangle, then label each cell
> of that rectangle with a digit from 1 to 5 such that each pentomino contains
> all of 1 through 5, and, whenever two adjacent cells belong to different
> pentominoes, their labels' sum is a prime number.

"marpocky" pointed out that the puzzle was unsolvable, because of the X pentomino:
no matter how you label its legs, you arrive at a contradiction. For example, if
two adjacent legs of the X are labeled 3 and 4, then we must have $$x$$ such that
$$x+3$$ and $$x+4$$ are both prime; this is impossible.

![The X pentomino](/blog/images/2024-01-15-x-pentomino.svg)

Likewise, adjacent legs cannot be labeled (2,3), (2,5), or (4,5).

* Suppose one of the legs of the X is labeled 2; then neither adjacent leg can be
either 3 or 5, so they must be 1 and 4 in either order. With 1,2,4 accounted for,
the fourth leg (opposite the 2) can't be either 3 (because (3,4) is verboten)
or 5 (because (4,5) is verboten). Ergo, none of the X's four legs can be labeled 2.

* Suppose one of the legs is labeled 3; then neither adjacent leg can be either
2 or 4 (because (2,3) and (3,4) are verboten), so they must be 1 and 5 in either
order. With 1,3,5 accounted for, the fourth leg (opposite the 3) can't be either
2 (because (2,5) is verboten) or 4 (because (4,5) is verboten). Ergo, none of the
X's four legs can be labeled 3.

Without 2 and 3, we're left with only three labels for the X's four legs; Q.E.D.,
the X can't be labeled with these digits at all.

----

However, "electricmaster23" quickly found [a solution](https://imgur.com/QvKXC8M)
to the following variation:

> Pack the 12 pentominoes into a 6x10 rectangle, then label each cell
> of that rectangle with a digit from 0 to 4 such that each pentomino contains
> all of 0 through 4, and, whenever two adjacent cells belong to different
> pentominoes, their labels' sum is a prime number. (Neither 0 nor 1 is prime.)

There are 2339 possible packings to consider (up to rotation and reflection).
I didn't quickly find a listing of all 2339 packings of the 12 pentominoes into
a 6x10 rectangle, so I've made my own listing, [here](/blog/code/2024-01-15-the-2339-6x10-pentomino-packings.txt).
It starts with the line

    FIIIIILLLLFFFNWWTTTLYFNNXWWTZZYYNXXXWTZVYUNUXPPZZVYUUUPPPVVV

which represents the packing

![The above packing](/blog/images/2024-01-15-example-packing.svg)

I wrote a little C program to find all the labelings of these 2339 graphs
satisfying the sum-to-a-prime criterion. There are a lot of them! For the packing above,
there are 12,347,672 possible labelings. This is the lexicographically first of them:

![A sample 01234-coloring](/blog/images/2024-01-15-example-coloring.svg)

When we consider not just this one packing but also the other 2338 possible packings,
we find that the total number of viable labelings satisfying the sum-to-a-prime criterion
is truly astronomical. So far my computer search has churned out 585,401,512 labelings
in which the upper right corner is occupied by the F pentomino; 1,116,710,101 where it's
occupied by the I pentomino in horizontal orientation; 2,557,931,515 where it's
occupied by the I in vertical orientation; 2,471,381,448 where it's occupied by the L;
1,182,035,984 by the N; 2,405,917,776 by the P; 2,419,068,152 by the T...
The total number of viable labelings is probably about 20 or 30 billion.

----

There are other labeling criteria we could investigate. Many possible
criteria are simply unsolvable, just like the 12345-coloring criterion we started with:

<b>Label with 1,2,3,4,5 such that adjacent cells belonging to different pentominoes
always sum to _k_ mod _n_.</b> (No solutions; consider the X tile.)

<b>Label with 1,2,3,4,5 such that adjacent cells belonging to different pentominoes
always differ by at least 3.</b> (No solutions; consider the X tile.)

<b>Label with 1,2,3,4,5 such that adjacent cells belonging to different pentominoes
always multiply to at least 6.</b> (No solutions; consider the F tile's 1-cell.)

But some are even more "interesting" (in the sense of having fewer solutions)
than the 01234-coloring criterion. For example:

<b>Label with 1,2,3,4,5 such that adjacent cells belonging to different pentominoes
always differ by at most 1.</b>
There are 1,059,492,120 labelings that satisfy this criterion. 517 of the 2339 possible
packings have no viable labeling at all. The packing with the fewest viable labelings
(at 64) is:

![A viable packing for the "differ-by-at-most-1" criterion](/blog/images/2024-01-15-differ-by-at-most-1.svg)

<b>Label with 1,2,3,4,5 such that adjacent cells belonging to different pentominoes
always differ by at least 2.</b>
There are 101,275,328 labelings that satisfy this criterion. 91 of the 2339 possible
packings have no viable labeling at all. The packing with the fewest viable labelings
(at 48) is:

![A viable packing for the "differ-by-at-least-2" criterion](/blog/images/2024-01-15-differ-by-at-least-2.svg)

<b>Label with 0,1,2,3,4 such that adjacent cells belonging to different pentominoes
always sum to a power of two. (0 is not a power of two.)</b>
There are only 240 labelings that satisfy this criterion. 2331 of the 2339 possible
packings have no viable labeling at all. Here's a representative labeling from
each viable packing:

![The viable packings for the sum-to-a-power-of-two criterion](/blog/images/2024-01-15-sum-to-powers-of-two.svg)

----

In fact, we can completely abstract the criterion-construction part: Label with
$$a_1, a_2, a_3, a_4, a_5$$ such that the labels $$(a_i, a_j)$$ of
adjacent cells belonging to different pentominoes satisfy $$R(a_i, a_j)$$,
for a given symmetric relation $$R$$. There are only 544 possible values
for $$R$$ (that's [OEIS A000666](https://oeis.org/A000666)), so we could just
run through them and see if any of them produce a thrilling puzzle.

[Here's Python code](/blog/code/2024-01-15-enumerate-symmetric-relations-on-n-elements.py)
to generate those 544 relations. But to exhaustively explore all 544 puzzles corresponding
to those 544 relations, you'll need a smarter algorithm and/or a faster computer than I've got!

----

Looking at these pictures, it strikes me that the "right" presentation for this kind of
puzzle would be to give the pentominoes cut free of the grid, with their cells still
labeled; then the puzzle-solver's task would be to fit them back into the grid such
that the criterion was satisfied.

![A disassembled puzzle](/blog/images/2024-01-15-puzzle-differ-by-at-least-2.svg)

Can you fit these pentominoes back into a 6x10 grid such that adjacent cells belonging to
different pentominoes <b>always differ by at least 2?</b>
(Some pieces must be rotated, none reflected.)

Now, a really clever puzzle-constructor would present a single set of pieces that could be
put back together in two different ways — one way satisfying the "at least 2" criterion
and another way satisfying the "at most 1" criterion. I'm afraid I don't have nearly that
much patience.
