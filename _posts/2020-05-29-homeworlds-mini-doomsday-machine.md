---
layout: post
title: "Homeworlds Puzzle: Mini Doomsday Machine"
date: 2020-05-29 00:02:00 +0000
tags:
  board-games
  homeworlds
  puzzles
  web
excerpt: |
  Ray's red homeworld is armed to a frankly ridiculous degree, but all for naught:
  Lee's mini-Doomsday-Machine is almost complete and his victory is assured.
---

The other day I discovered that Puzzling StackExchange has a few
"Homeworlds puzzles" — just like chess puzzles, but using the game
of [Binary Homeworlds](https://boardgamegeek.com/boardgame/14634/homeworlds) instead of chess.

After spending much too much time trying to solve Sleafar's
["mate in 4" puzzle](https://puzzling.stackexchange.com/questions/57084/homeworlds-monopoly)
from 2017 (my conclusion: unsolvable as far as I could tell), I
decided to try my hand at creating a puzzle. In fact, I created
two! Here's the first of them.

## Creating the puzzle

Two things were indispensable:
my physical Homeworlds set and my [computerized position explorer](https://github.com/Quuxplusone/Homeworlds).
I wrote a quick and dirty program to verify the properties I wanted my puzzle to have —
most importantly "has exactly one move leading to checkmate," but also "has at least one of
these various kinds of move, which all lead to check-but-not-checkmate." Then, as I moved
the pieces around in meatspace, focusing on one snippet of the problem at a time, I would type
the position into the computer to quickly check that I hadn't lost some other desirable property
by accident.

Of course, if you have access to a computer program like mine, "solving" a puzzle
like this one becomes completely trivial! The enjoyment in this puzzle — if any :) — will come
from solving it without computer assistance.

For those who want to get a physical Homeworlds set like mine, your best bet right now is Looney Labs'
[_Pyramid Arcade_](https://www.looneylabs.com/games/pyramid-arcade) (or [buy on Amazon](https://amzn.to/36HRkGx)).
They once sold sets suitable for Homeworlds under the name ["Rainbow Stash"](https://amzn.to/2zMe6B5);
but that's been out of print for a long time. Still, there's good news for Homeworlds fans:
Looney Labs will start selling
[dedicated standalone Homeworlds sets](https://www.looneylabs.com/news/pyramid-quartet-kickstarter)
circa October 2020!

All credit for the lovely image below is due to Sleafar on Puzzling StackExchange,
who created [an SVG image template](/blog/images/2020-05-29-3vd.svg) where each piece
is a different grouped entity that you can just drag around in an SVG editor until
it looks right. The wonders of modern technology!

I used [Boxy SVG](https://boxy-svg.com/app), a JavaScript-based online SVG editor,
to do the dragging-around. (Back when I was working on _Colossal Cave: The Board Game_,
I knew how to use Inkscape; but those brain cells have died.) Boxy lets you upload
SVG files from disk; it requires registration in order to download images as SVG or even
as PNG. For my purposes, I just took a screenshot and cropped it.


## The puzzle

![A graphical representation of the game state.](/blog/images/2020-05-29-mini-doomsday-machine.png)

    Lee (0, g3b2) r1r3g1b1-
    Ray (1, r1r3) -y2g3b3
    DS1 (y2) b2-r1r3
    DS2 (g1) g1g2-
    DS3 (g2) y2-
    DS4 (b2) r2-g3

The stash contains `r2r2 y1y1y1y3y3y3 g2 b1b1b3b3`.

Ray's red homeworld is armed to a frankly ridiculous degree, but all for naught:
Lee's mini-[Doomsday-Machine](https://jpeterbaker.github.io/homeworlds/site/glossary.html#doomsdayMachine)
is almost complete and his victory is assured.

Notice that Lee can threaten Ray via moves "themed" in any of the four colors!
Lee can deliver check by <b>attacking</b> a red ship at DS1, or by <b>moving</b> a red ship to Ray,
or by <b>building</b> a red ship at DS4, or by <b>trading</b> for a red ship at DS2...
but nevertheless, there is only one path to the quick checkmate.

Lee to play and mate in 1. (That is, you must find the unique move which Lee can make,
such that no matter what Ray replies, Lee will win on the very next turn.)

----

For the solution, see [the discussion of this puzzle on BoardGameGeek](https://boardgamegeek.com/thread/2436984/).
