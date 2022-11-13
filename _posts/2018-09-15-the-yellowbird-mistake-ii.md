---
layout: post
title: 'Homeworlds: The Yellowbird Mistake, Part II'
date: 2018-09-15 00:01:00 +0000
tags:
  board-games
  homeworlds
---

Following up on [my previous post](/blog/2018/09/10/the-yellowbird-mistake) titled "The Yellowbird Mistake"...

First of all, the Bluebird in Amsterdam is a [coffeeshop](https://en.wikipedia.org/wiki/Coffeeshop_(Netherlands)),
not a tavern. The editors regret the error.

Several commenters on BoardGameGeek (including Andy Looney himself!) opine that the name "Yellowbird Mistake"
is cute but totally misleading. Fair enough. But where the naming discussion has so far focused on the
big green sacrifice as the key aspect of the Yellowbird (or whatever), I think the more salient aspect for
me — certainly the salient aspect psychologically — is that the move usually results in the attacker's
cornering the market in some *non*-green color. To the defender, the salient color is the one you just
got locked out of! So I've been thinking of this move as primarily blue-colored.

Speaking of statistics: Writing that post made me embarrassed that I'd been running my "statistical analysis"
on only 699 games, the most recent of which was played more than eight years ago. So it motivated me to
go write a scraper (using the Python
[`requests`](http://docs.python-requests.org/en/master/user/quickstart/#make-a-request) library)
to fetch *all* the raw game transcripts from the SDG archives, and then update the script I used to
"cook" those transcripts into a format acceptable to my analysis program, and then collect new stats
with this much larger sample set.
All of this code is now available [on my GitHub](https://github.com/Quuxplusone/Homeworlds/).

Out of 3281 raw transcripts, 2799 of them describe two-player games where at least one move
was made after the homeworlds were set up. Of those 2799 games, 2755 follow the rules of
[my Homeworlds analysis program](https://github.com/Quuxplusone/Homeworlds/).
The other 44 are considered invalid by my scripts, either because they exploited bugs in the SDG game engine which
have since been fixed, or because they exploit a difference between the SDG engine and mine which to my
knowledge is still legitimately _de gustibus_: are catastrophes allowed only at the end of either player's
turn (my engine's rule) or anytime during the turn (SDG's engine's rule)?

Of those 2755 two-player games, 216 of them (7.8%) contain a [Bluebird](/blog/2018/09/10/the-yellowbird-mistake/#today-i-wrote-a-quick-and-dirty) move.
11 of them contain two Bluebird moves. 53 of these 227 Bluebird moves (23.3%) were game-winning moves, and in
21 of those 53 cases (39.6%) the losing player had had more ships than the winner.

Meanwhile, of those 2755 two-player games, 189 of them (6.9%) contain a [Yellowbird](/blog/2018/09/10/the-yellowbird-mistake/#i-wrote-a-second-quick-and-dirty) move.
16 of them contain two Yellowbird moves. 3 of them contain three such moves.

----

So, given this larger sample size, what can we say about the primary color of the Yellowbird move?
What is the most frequent color of the resulting monopoly? To start with, we'll use the color
of the newly built large ship as a proxy for the monopoly color:

- In 107 of our 208 Yellowbird moves (51.4%), the color of the newly built large ship is blue.

- In 81 of our 208 Yellowbird moves (38.9%), the color of the newly built large ship is yellow.

- In 20 of our 208 Yellowbird moves (9.6%), the color of the newly built large ship is red.

By definition, the newly built large ship is never green, because our definition of "Yellowbird move"
required that the sacrificed g3 be the *only* green ship at the attacker's homeworld.

Then, we look at which colors are "accessible" to the defender. Can the defender build, capture,
or trade for any ship of the given color? Vice versa, if it were still the attacker's turn, could
*they* build, capture, or trade for a ship of the given color? If the attacker could get a ship of
a certain color but the defender can't, then we'll somewhat arbitrarily say that the attacker
has a "monopoly" on that color. Here's the breakdown of the situations immediately following all
of our identified Yellowbird moves. (This table double-counts multiple monopolies.)

|  Color of new large ship:     | Red | Yellow | Blue |
| ----------------------------- | --- | ------ | ---- |
|  Attacker has red monopoly    |  2  |    2   |   2  |
|  Attacker has yellow monopoly |  .  |   10   |   6  |
|  Attacker has green monopoly  |  .  |    1   |   .  |
|  Attacker has blue monopoly   |  .  |    2   |  39  |
|  Attacker has no monopolies   | 18  |   67   |  63  |

So indeed I'm still [seeing confirmation](https://en.wikipedia.org/wiki/Confirmation_bias)
that "blue" is the dominant color associated with this tactic.

----

Getting back to the naming issue, Andy Looney writes that

> the name should really refer to the act itself ... rather than [the]
> failure to notice what the other guy is doing

As this tactic is often the fast track to a monopoly, maybe we should call it the
[Jerome Jacobson](https://www.thedailybeast.com/how-an-ex-cop-rigged-mcdonalds-monopoly-game-and-stole-millions)?
