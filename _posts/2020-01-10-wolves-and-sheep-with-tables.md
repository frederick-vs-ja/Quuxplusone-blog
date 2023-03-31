---
layout: post
title: 'Wolves and Sheep, with tables'
date: 2020-01-10 00:01:00 +0000
tags:
  math
  puzzles
---

Previously on this blog: ["Wolves and sheep"](/blog/2019/04/17/wolves-and-sheep/) (2019-04-17).
Since April, I've learned that this puzzle is exactly equivalent to "What is the smallest
number $$t$$ of rows that a $$n$$-column matrix can have and remain [$$d$$-separable](https://en.wikipedia.org/wiki/Disjunct_matrix)?"
As with the ["Discrete Peaceful Encampments" puzzle](/blog/2019/10/18/discrete-peaceful-encampments-with-tables/)
(2019-10-18), the answers to this question can be tabulated as a triangular matrix:

    d=       1  2  3  4  5  6  ...
          .
    n=1   .  0
    n=2   .  1  0
    n=3      2  2  0
    n=4      2  3  3  0
    n=5      3  4  4  4  0
    n=6      3  5  5  5  5  0
    n=7      3  6  6  6  6  6  0
    n=8      3  6  7  7  7  7  7  0
    n=9      4  7  8  8  8  8  8  8  0
    n=10     4  7  9  9  9  9  9  9  9  0
    n=11     4  8 10 10 10 10 10 10 10 10  0
    n=12     4  8 11 11 11 11 11 11 11 11 11  0
    n=13     4  8 12 12 12 12 12 12 12 12 12 12  0
    n=14     4  9 12 13 13 13 13 13 13 13 13 13 13  0
    n=15     4  9 13 14 14 14 14 14 14 14 14 14 14 14  0
    n=16     4  9  .  . 15 15 15 15 15 15 15 15 15 15 15  0
    n=17     5  9  .  . 16 16 16 16 16 16 16 16 16 16 16 16  0
    n=18     5 10  .  .  . 17 17 17 17 17 17 17 17 17 17 17 17  0
    n=19     5 10  .  .  . 18 18 18 18 18 18 18 18 18 18 18 18 18  0
    n=20     5 10  .  .  .  . 19 19 19 19 19 19 19 19 19 19 19 19 19  0

UPDATE, 2023-03-31: The non-zero elements of this triangle are now [OEIS sequence A361928](https://oeis.org/A361928).

Column $$d=1$$ is simply $$\lceil\lg{n\choose k}\rceil$$.

Column $$d=2$$ is related to [OEIS sequence A054961](https://oeis.org/A054961),
the maximal number of columns $$n$$ in a 2-separable matrix with $$t$$ rows.
Dmitry Kamenetsky has determined that $$t(22, 2)\le 10$$, $$t(31, 2)\le 11$$, and $$t(46, 2)\le 12$$.
Zhao Hui Du adds that $$t(55, 2)\le 13$$, $$t(63, 2)\le 14$$, etc.; see the OEIS for more details.

Column $$d=3$$ is related to [OEIS sequence A290492](https://oeis.org/A290492),
the maximal number of columns $$n$$ in a 3-separable matrix with $$t$$ rows.

Many, many thanks to [Elaqqad](https://stackoverflow.com/users/4752165/elaqqad) over on Math.SE
for helping me come to these realizations, and for providing sample solutions for $$t(111,3)\le 37$$
and $$t(104,5)\le 59$$.

As with "Discrete Peaceful Encampments," I provide a text file of best known solutions
for many different $$n$$ and $$d$$ (not just the integer $$t(n,d)$$ for each pair, but an actual
$$t\times n$$ matrix which is verifiably $$d$$-separable).
You can find the current file [here](/blog/code/2020-01-10-wolves-and-sheep-best-results.txt).

Unlike "Discrete Peaceful Encampments," _many_ of the solutions to "Wolves and Sheep" are trivially
derived from the solutions to "adjacent" instances of the problem. For example, the solution for
$$t(n, d)$$ often looks very much like the solution for $$t(n+1, d)$$ with one column missing;
and the solutions for $$t(n, 1)$$ all follow a simple pattern. So the solution file contains only
those solutions which are "irreducible" — which can't be expressed by simple manipulations of other
solutions. I wrote a simple program to read in the solution file, apply those simple manipulations,
and print out the best known solution for any $$(n,d)$$.

    $ ./wolfy --verify 14 3
    Candidate is
    N=14 D=3 T=12 guaranteed_best=0
    1.1.....1...1.
    ...1.1......11
    .....11.11....
    ..1.11....1...
    ....1...1..1.1
    .11......1...1
    ..11..1....1..
    .1.1....1.1...
    1.....1...1..1
    1..11....1....
    .........1111.
    .1..1.1.....1.

    Verified. This is a solution for t(14, 3) <= 12.

My file contains some very large instances for $$d=2$$ developed by Zhao Hui Du; the largest at the
moment is $$t(1090, 2)\le 27$$. See his blog post ["两瓶毒酒问题"](https://emathgroup.github.io/blog/two-poisoned-wine)
(October 2019); the data file is found at the last link on that page (currently
[here](https://emathgroup.github.io/d7c7ee4b784829e5df9268fa6cd338cf/poisonall.txt)).

I'd be interested to see some more examples of "irreducible" $$d$$-separable matrices for small $$n$$.
If you know where to find such examples, please send them my way! I'll add them to my text file
[here](/blog/code/2020-01-10-wolves-and-sheep-best-results.txt).

----

Find further StackExchange discussions
[here](https://puzzling.stackexchange.com/questions/100999/finding-the-number-of-poisoned-bottles),
[here](https://math.stackexchange.com/questions/639/logic-problem-identifying-poisoned-wines-out-of-a-sample-minimizing-test-subje),
and
[here](https://mathoverflow.net/questions/59939/identifying-poisoned-wines).
