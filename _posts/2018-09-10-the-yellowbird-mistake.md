---
layout: post
title: 'Homeworlds: The Yellowbird Mistake'
date: 2018-09-10 00:01:00 +0000
tags:
  board-games
  homeworlds
---

This post is about [Binary Homeworlds](https://boardgamegeek.com/boardgame/14634/homeworlds), the "space chess" game invented by
John Cooper and popularized by (among others) [Andy Looney](http://www.wunderland.com/WTS/Andy/Games/ILoveHomeworlds.html).
You can play Homeworlds online at [SuperDuperGames](http://superdupergames.org/gameinfo.html?game=homeworlds).
Sadly a full rules explanation is out of scope for this blog post, but maybe sometime.
In this post I just want to talk about a specific tactical error that (sigh) I keep making.

Aficionados of Homeworlds will have heard of the "[Bluebird Mistake](https://www.youtube.com/watch?v=081STRjOFMw)."
Andy Looney invented the name for this pattern; it's named for the Bluebird coffeeshop in Amsterdam
where Andy fell victim to it. Basically it's where you look at the board and think, "My homeworld
is safe from overpopulation because I have only two pieces of any one color there; *three* pieces
is where it gets dangerous. Also, my opponent only has one piece within striking distance."
And then on the next move, your opponent sacrifices y3 to move two ships into your homeworld
(one of those ships moving *two hops*, which is how it escaped your notice originally), and
boom goes the star system.

Today I wrote a quick-and-dirty filter to scan through my
[archive of 699 games (2005–2009)](https://github.com/Quuxplusone/Homeworlds/tree/master/superdupergames-archive)
for cases of the following pattern, which I will call a "Bluebird move":

- The move consists of a y3 sacrifice, three moves, and a catastrophe.

- The catastrophe is at the defender's homeworld.

- The defender's homeworld had only two pieces of the catastrophed color.

- Both of the ships moved are of the catastrophed color, and one of them moves two hops.

Of my archive's 699 games, 18 of them were won via a Bluebird move.
In 5 of those 18 games, prior to the final catastrophe, the losing player actually had
more ships on the board than the winner did!

Looking at games where a Bluebird move *happened* but was not necessarily
the *winning* move, we find 77 instances across 73 unique games. (That is, four games saw this
pattern arise repeatedly.)

----

But I wanted to talk about a different pattern, one that to my knowledge does not have
an accepted name yet. Basically *this* one is where you look at the board and think,
"I don't have to worry about my opponent building material on his turn; he
won't dare sacrifice his sole g3 at home since he's unable to rebuild it. So the best he can
do is get a medium ship somewhere." And then on the next move, your opponent *does* sacrifice
his g3 to build that ship you were looking at, plus two more ships, one of which is a large of
a *different* color (let's say, blue) in his home system. So he's still got a big ship at home
after all — it's just changed colors from green to blue — and now he's got a monopoly on blue
and he's going to be farming big ships for the rest of the game (which is likely to be
nasty, brutal and short).

[Here's](http://superdupergames.org/?page=archive_play&gid=34553&idx=13) the position in my most recent game where I let this happen to me.

    ajo (0, y3b2) g3y1-
    Felix (1, r1b2) -y1b1g3
    Rim (g3) -b1
    Alpha (b1) g1y1-

    Stash: r1r1r2r2r2r3r3r3y2y2y2y3y3g1g1g2g2g2b2b3b3b3

    ajo:   build g1 at ajo
    Felix: sacrifice g3 at Felix; build b2b3 at Rim; build b3 at Felix (!)

For obvious reasons, I have decided to call this tactical error the "Yellowbird Mistake."

I wrote a second quick-and-dirty filter to scan through my archive of 699 games
for cases of the following pattern, which I will call a "Yellowbird move":

- The move consists of a sacrifice at home and three builds.

- The sacrificed g3 is both the only green, and the only 3, in the attacker's home system.

- At the start of the turn, there are no 3s exposed in the stash.

- At the end of the turn, the attacker once again has a 3 at home, and the defender has no way to build any 3 of his own (not even by sacrificing).

Of my archive's 699 games, we find 49 Yellowbird moves across 42 unique games.
(Five games saw this pattern arise repeatedly.)  In [only one case](http://superdupergames.org/?page=archive_play&gid=1695&idx=66)
was the resulting advantage immediately erased by a catastrophe.  In 26 of the 49 cases,
the result was that the attacker gained a monopoly on the color they'd just built — meaning
that the defender had no way to build, swap for, or capture any ship of that color on their next turn.
In 14 of those 26 cases, that monopoly was in blue.

(In general this would just be because the stash was void in that color. My analysis doesn't tell
us anything about the imperviousness, or otherwise, of such monopolies.)

The moral of the story, such as it is, is: Watch out for unexpected green sacrifices.
One doesn't need a factory to become a monopolist!

----

See [the followup to this post](/blog/2018/09/15/the-yellowbird-mistake-ii).
