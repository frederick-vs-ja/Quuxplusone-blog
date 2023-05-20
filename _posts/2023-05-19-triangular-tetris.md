---
layout: post
title: "Triangular Tetris"
date: 2023-05-19 00:01:00 +0000
tags:
  celebration-of-mind
  playable-games
  web
excerpt: |
  At the Gathering 4 Gardner weekly meetup a few weeks ago, we were discussing
  the [aperiodic monotile](/blog/2023/03/30/hat-parquet/) and someone proposed
  a variation of Tetris where the only shape you get is the hat.
  I was inspired to start work on something like that — I just haven't yet
  been inspired to finish! I only got as far as changing Tetris's square grid
  to a triangular — that is, polyiamond — grid.

  I present: [Triangular Tetris!](https://quuxplusone.github.io/HatTetris/triangular.html)
---

At the Gathering 4 Gardner weekly meetup a few weeks ago, we were discussing
the [aperiodic monotile](/blog/2023/03/30/hat-parquet/) and someone proposed
a variation of Tetris where the only shape you get is the hat.
I was inspired to start work on something like that — I just haven't yet
been inspired to finish! I only got as far as changing Tetris's square grid
to a triangular — that is, polyiamond — grid.

I present: [Triangular Tetris!](https://quuxplusone.github.io/HatTetris/triangular.html)

The source code (as always, available [on my GitHub](https://github.com/Quuxplusone/HatTetris))
is based on Jake Gordon's [javascript-tetris](https://github.com/jakesgordon/javascript-tetris)
from 2011. Jake [wrote a blog post](https://codeincomplete.com/articles/javascript-tetris/)
dissecting his code in detail.

I'm not terribly happy with the rotation controls; if you play it right now you'll see
what I mean. I might revisit it and come up with something better, later. Also right now
you'll see that the falling piece can "clip through" stationary pieces in at least two
unusual ways, one of which actually involves one triangle being drawn partially behind
another. I don't know whether to call that a bug or a feature.

To turn this into Hat Tetris, one would need to divide each of those triangles into three
kites; figure out collision detection; figure out intuitive rotation controls; and (hardest
of all, I think) figure out what "completing a row" looks like. I have some ideas;
just have to find time to implement them...

----

For another interpretation of "triangular Tetris," check out David Bagley's
[Tertris](https://sillycycle.com/altris.html), which is also [playable online](https://sillycycle.com/html5/Tertris.html).
