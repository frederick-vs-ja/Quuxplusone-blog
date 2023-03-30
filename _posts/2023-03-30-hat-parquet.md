---
layout: post
title: "Escheresque parquet deformations of an aperiodic monotile"
date: 2023-03-30 00:01:00 +0000
tags:
  celebration-of-mind
  math
  pretty-pictures
  web
---

On March 20, it was announced that a team of mathematicians [has found](https://arxiv.org/abs/2303.10798)
a 13-sided polygon that can tile the plane aperiodically (and that _cannot_
tile the plane periodically), thus solving the so-called
"[einstein problem](https://en.wikipedia.org/wiki/Einstein_problem)."
[Craig Kaplan's website](https://cs.uwaterloo.ca/~csk/hat/)
has lots of details on the new shape — which so far has generally been
dubbed the "hat," even though to me it looks a lot more like a T-shirt.

One of the cool details about the "hat" is that it's just one of a whole
continuous family of shapes, each of which is still an aperiodic monotile.
Here are seven representatives of that set: the first, fourth, and seventh
are special cases that can tile either periodically or aperiodically, while
the other four are representatives of the infinite family that tile only
aperiodically. The fifth is the "hat"; the first, third, and seventh are known
as the "comet," "turtle," and "chevron" respectively.

![A family of monotiles](/blog/images/2023-03-30-monotile-family.png)

This got me thinking about M.C. Escher's [_Metamorphosis_ series](https://www.escherinhetpaleis.nl/story-of-escher/metamorphosis-i-ii-iii/?lang=en).
Wouldn't it be interesting to make a tessellation that morphs smoothly from
one member of this family to another? (In fact, einstein coauthor Craig Kaplan,
mentioned above, also happens to be [big into parquet deformations!](https://www.theguardian.com/artanddesign/alexs-adventures-in-numberland/2014/sep/09/crazy-paving-the-twisted-world-of-parquet-deformations))

![Detail of "Metamorphosis II"](/blog/images/2023-03-30-metamorphosis-ii-detail.jpg)

So I wrote a little JavaScript thingie you can run in your browser to
create your own Escher-like parquet deformations. Play with it
[here](/blog/code/2023-03-30-hat-parquet.html), to produce on-screen
images like this one:

![Escheresque parquet deformation](/blog/images/2023-03-30-parquet.png)

What would Escher do with this pattern?
