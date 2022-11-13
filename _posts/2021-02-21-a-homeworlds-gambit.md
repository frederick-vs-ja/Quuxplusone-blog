---
layout: post
title: 'Homeworlds: Analysis of an Opening Gambit'
date: 2021-02-21 00:01:00 +0000
tags:
  board-games
  homeworlds
---

This post is about [Binary Homeworlds](https://boardgamegeek.com/boardgame/14634/homeworlds), the "space chess" game invented by
John Cooper and popularized by (among others) [Andy Looney](http://www.wunderland.com/WTS/Andy/Games/ILoveHomeworlds.html).
You can play Homeworlds online via email at [SuperDuperGames](http://superdupergames.org/gameinfo.html?game=homeworlds),
and, as of this year, in real time at [Homeworlds Live](https://homeworlds-live2.glitch.me/lobby).
Jonathan Baker runs [an annual tournament](https://jpeterbaker.github.io/homeworlds/site/tournament/tmain.html),
which is happening as we speak.

In one of my tournament matches I observed an interesting opening [gambit](https://en.wikipedia.org/wiki/Gambit).
In Homeworlds, the early game is all about the economy. The players are (frequently) racing to build the first
medium ship. Jonathan Baker has described an aggressive opening tactic called the
["Instafreeze"](https://jpeterbaker.github.io/homeworlds/site/tactics.html#instafreeze):
when Lee's homeworld contains a small piece, Ray can take the second small as well as a big ship
in that color, thus freezing Lee out of that color unless Lee trades his large ship.

![Instafreeze in green.](/blog/images/2021-02-21-instafreeze-in-green.png){: .meme}

The instafreeze happens during setup, and is driven entirely by Ray; Lee's role is simply reactive.
This next gambit — the one I noticed in the tournament — is set in motion by Lee, and is interactive.
But it begins similarly: Lee's homeworld contains a small piece, and Ray is going to use that to
threaten a jump-start on that color's economy.

> This gambit works with either red or yellow, but right now I'm going to present it in red,
> for the sake of concreteness.

When Lee's homeworld contains r1, Ray can (either deliberately or coincidentally)
_avoid_ red in his own homeworld. Ray's natural second move will be to swap for r1, that being
the one color he doesn't yet own. This leaves a single r1 in the stash. Ray's third move,
if Lee lets him, is to build a second r1 at home. Boom! Lee is frozen out of red —
_and_ Ray has only two reds at home, meaning he's safe to build r2 on his fourth move!

This is the happy path for Ray: Lee just completely let him do his own thing. This is obviously
_not_ the happy path for Lee. So let's see what Lee should do to throw a wrench into Ray's plans.

If Lee notices that his own homeworld contains r1, and Ray's has no red, then on turn two
Lee can trade for r1 — a color already present at his homeworld! This leaves a single r1
in the stash. <b>Gambit offered.</b>

If Ray trades for r1 (remember, this would be a perfectly natural second move), <b>gambit accepted.</b>
Lee gets to build the first r2 — but Ray gets to build r2 right afterward. We expect a flurry
of trades and builds ("trade r2 for...; build r2; trade r2 for...") as both players rapidly
develop a fleet of medium ships.

Alternatively, Ray might decide that rapid development isn't his style, and respond by
building another small in the color he's got; or by trading instead for r3, which keeps Ray in
the red economy without actually opening up the r2s for Lee. (And Lee certainly isn't
going to open them up for Ray!) <b>Gambit declined.</b> In this case, Lee has maybe lost
a little time; but he's shut off Ray's path to a quick r2, and I think it ends up
pretty even. The development continues much more "quietly" than if Lee's gambit had
been accepted.

Here are two representative game trees — one in yellow, one in red. In both cases,
player 1 (South) was the first to move. On the left: happy path for North.
In the middle: Gambit declined. On the right: Gambit accepted.

|:-----:|:------:|
| [![Gambit in yellow.](/blog/images/2021-02-21-gambit-in-yellow.png)](/blog/images/2021-02-21-gambit-in-yellow.png) | [![Gambit in red.](/blog/images/2021-02-21-gambit-in-red.png)](/blog/images/2021-02-21-gambit-in-red.png) |

I wrote a quick-and-dirty filter to scan through my
[archive of 2755 games (2005–2018)](https://github.com/Quuxplusone/Homeworlds/tree/master/superdupergames-archive)
for cases of the following pattern:

- The first player's second move is a trade for small ship.

- The ship's color is the same as a small piece in the star.

- The second player's homeworld contains no piece of that color.

- The second player *does* have access to blue (otherwise the gambit could not be accepted).

Here are the results, enumerated and tabulated:

    Gambit in red declined:
    10689 11166 11324 22127 25723

    Gambit in red accepted:
    18850 19292 24121 24792 25751 25999 26568 28620 29703 32625 33090 33515 33849 33871

    Gambit in yellow declined:
    3728 3766 3835 3956 4226 7612 7701 8148 8244 8255 8271
    14412 20639 22897 27486 29701 32758 33591 34158

    Gambit in yellow accepted:
    7273 7613 7628 7838 8264 9466 9717 10771 11161 15643
    16452 17298 17340 17429 17783 17960 18413 18816 20572 21986 22165 22585 22767
    23342 23911 23986 25319 25397 26773 28444 29921 30256 30712 31130 31265 31951
    32198 32479 32886 32952 33478 33859 34052 34112 34191 34202 34203 34308 34497

| Color   | Offered | Accepted | Declined |
|---------|--------:|---------:|---------:|
| red     | 19      |       14 |        5 |
| yellow  | 68      |       49 |       19 |
| total   | 87      |       63 |       24 |
