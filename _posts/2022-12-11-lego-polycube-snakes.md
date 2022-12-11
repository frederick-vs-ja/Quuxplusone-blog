---
layout: post
title: "Lego polycube snakes"
date: 2022-12-11 00:01:00 +0000
tags:
  lego
  math
  pretty-pictures
  puzzles
excerpt: |
  Previously on this blog:

  * ["Polycube snakes and ouroboroi"](/blog/2022/11/18/polycube-snakes/) (2022-11-18)
  * ["Polyomino strips, snakes, and ouroboroi"](/blog/2022/12/08/polyomino-snakes/) (2022-12-08)

  I've bought enough Lego bricks to play around with polycube snakes in real life
  (just in time for Christmas!). Here are some curiosities I've constructed so far.
---

Previously on this blog:

* ["Polycube snakes and ouroboroi"](/blog/2022/11/18/polycube-snakes/) (2022-11-18)
* ["Polyomino strips, snakes, and ouroboroi"](/blog/2022/12/08/polyomino-snakes/) (2022-12-08)

I've bought enough Lego bricks to play around with polycube snakes in real life
(just in time for Christmas!). Here are some curiosities I've constructed so far.

----

Back in 2019, Roy van Rijn investigated the maximum length of a snake packed into an $$m\times n$$
rectangle (["The longest maze/snake,"](https://www.royvanrijn.com/blog/2019/01/longest-path/) January 2019;
[OEIS A331968](https://oeis.org/A331968) is related).
Recently he made a neat JavaScript visualizer for 3D snakes packed into $$n\times n\times n$$ cubes:
[3x3x3](https://www.royvanrijn.com/polycube/3x3x3.html),
[4x4x4](https://www.royvanrijn.com/polycube/4x4x4.html), [5x5x5](https://www.royvanrijn.com/polycube/5x5x5.html),
[6x6x6](https://www.royvanrijn.com/polycube/6x6x6.html), [7x7x7](https://www.royvanrijn.com/polycube/7x7x7.html).
Here's a Lego model of his 36-cell 4x4x4 snake.

![4x4x4 packed snake](/blog/images/2022-12-11-4x4x4-snake.jpg)

----

I conjecture that the smallest ouroboros enclosing a "pseudo-tunnel" (as defined [here](/blog/2022/12/08/polyomino-snakes/#holes-in-3d))
is this picturesque heart-shaped ouroboros with $$n=22$$:

![Sam's heart-shaped ouroboros](/blog/images/2022-12-08-sams-heart-polycube.jpg)

The cubes on the border of the pseudo-tunnel can be popped up or down in four
different ways. Manipulating even further gives a "crumpled heart": a 22-cube ouroboros
with a "corner-type" pseudo-tunnel.
Here are two copies of that ouroboros, built with the studs on opposite faces.

![Crumpled-heart ouroboros](/blog/images/2022-12-11-crumpled-heart-polycube.jpg)

Can you produce any shorter ouroboros with a pseudo-tunnel?

----

I was surprised to find, in the output of my snake-enumerating program, that the
number of _one-sided_ non-ouroboros snakes with cavities was (so far) always twice the number
of _free_ non-ouroboros snakes with cavities; that is, my program was claiming that
all non-ouroboros snakes with cavities were chiral (up to $$n=18$$, at least).
So I went and manually constructed a 22-cube non-chiral non-ouroboros snake with
two cavities. The green snake was initially built as a mirror image of the red snake;
but flipping the green snake upside-down makes it a translated copy of the red snake
instead.

|:---:|:---:|
| ![As built](/blog/images/2022-12-11-nonchiral-cavitous-snake-1.jpg) | ![Rotated](/blog/images/2022-12-11-nonchiral-cavitous-snake-2.jpg) |

Martin Gardner presents a much simpler example of a "shape that has no plane of symmetry
yet can be superimposed on its mirror reflection" in his _Mathematical Games_ column of
June 1981 ([archive.org](https://archive.org/details/lastrecreationsh00gard_0/page/257/mode/2up),
[JSTOR](https://www-jstor-org.wikipedialibrary.idm.oclc.org/stable/24964433?seq=8)).
Personally I find these shapes hard to wrap my head around, even when I'm seeing them
in real life.

Is this the shortest non-chiral cavitous non-ouroboros snake, or can you find a shorter one?
(The "non-ouroboros" part is key, since it's not hard to find a mirror-symmetric cavitous ouroboros
of length 12.)

Is this the shortest snake with two cavities?

Can you produce a non-chiral non-ouroboros snake (of any length) with only _one_ cavity?

----

What is the shortest ouroboros with both a tunnel _and_ a cavity? Both a pseudo-tunnel and a cavity?

----

Ever since I decided to classify snakes by whether they had cavities or no cavities,
my C++ program has spent most of its CPU cycles trying to figure out if each snake has
a cavity or not. I have thought of two algorithms and two quick-reject heuristics:

- Find the bounding box of the snake, add borders on all six sides, and flood-fill starting
    from one corner. If the flood-fill reaches all `(volume - n)` empty cells,
    the snake is cavity-free.

- Or: Find the "perimeter" of the snake, consisting of the 24 or 25 empty neighbors of each
    occupied cell. Deduplicate and flood-fill. If the flood-fill reaches all `(volume - n)` empty cells,
    the snake is cavity-free.

- A "deflated" snake can't have cavities: If the snake's bounding box is less than 3 units in any dimension,
    the snake must be cavity-free.

- A "mostly straight" snake can't have cavities: If the snake's bounding box is _greater_ than `n - 8`
    units in any dimension, the snake must be cavity-free, because it takes 8 cubes to "tie a knot"
    around an empty cell in the middle of a straight snake. In the picture below, 9 green cubes enclose
    a single empty cell; this is an 18-cube snake in a 3x3x10 bounding box.

    ![This is not a drawing of a hat](/blog/images/2022-12-11-snake-with-knot.jpg)

- A "mostly straight" snake can't have cavities, round 2: If the snake has fewer than 6 turnings,
    then it must be cavity-free. In fact I _think_ any cavitous snake must have at least _seven_
    turnings; but I have no hard proof of that conjecture.

The quick-reject heuristics all suffer from the problem that the overwhelming majority of lengthy
snakes are neither "deflated" nor "mostly straight."
It would sure be nice to find a faster way to tell whether a snake has cavities or not.
