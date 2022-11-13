---
layout: post
title: "Antisocial Homeworlds, and a solitaire challenge"
date: 2021-04-04 00:01:00 +0000
tags:
  board-games
  homeworlds
  puzzles
---

A couple of weeks ago, I came up with a new variant of Binary Homeworlds.
Readers of this blog will already know Homeworlds as the "space chess"
game played with colorful pyramids, invented by John Cooper, and
popularized mainly by Andy Looney.
In fact, this variant was directly inspired by a variant
invented by Andy Looney in February 2021, which he called "Peaceful Homeworlds,"
and which you can read about [here](http://new.wunderland.com/2021/02/03/peaceful-homeworlds/).

> Full disclosure: I have not attempted to play "Peaceful Homeworlds,"
> but I think I wouldn't like it. It seems more complicated than the original,
> with lots of fiddly little corners and clarifications needed. But maybe
> after I post my own variant, the Internet will inform me that mine is
> _also_ full of holes! We'll see.

In Andy's "Peaceful Homeworlds" there is no capturing allowed; the goal is to
score points by "mining crystals," and the ships of different players can
coexist peacefully together in any star system. Er, semi-peacefully: although
_direct attacks_ are verboten, it's still legal (and apparently sometimes worthwhile)
to overpopulate a system and cause a catastrophe, thus destroying some of your
fellow player's ships.

In my variant, such dirty tricks are prevented by the simple expedient of
forbidding ships of different players ever to occupy the same system.
So you can't blow up the other player's ships because you can never get next to them;
and you can't capture them for the same reason.
(Just to keep things simple, self-catastrophe'ing is also explicitly forbidden.)
I call this variant "Antisocial Homeworlds." In hindsight, I realize that
this leaves the much more timely name "Socially Distanced Homeworlds" available
for the next variant-inventor to grab.


## Rules of Antisocial Homeworlds

See the BoardGameGeek thread on Antisocial Homeworlds [here](https://boardgamegeek.com/thread/2620770/antisocial-homeworlds).

Play Homeworlds by all of the ordinary rules, plus these three new rules.

<b>(1: No Touching)</b> You may never move your own ship into a system
which is already occupied by someone else's ship.

* Not even to "pass through" on your way to somewhere else.

* Not even to cause a catastrophe that leaves the system in your hands alone.

<b>(2: No Overpopulations)</b> You may never put four pieces of the same color
together in one system, even for an instant.

* Not even to "pass through" on your way to somewhere else.

<b>(3: Win on Points)</b> At the instant that the stash first becomes empty,
the player controlling the most pips of material wins the game.

* By "pips of material," I mean that you add up the pips on all of your ships
    _and_ all of your stars. (Note that "your stars" is unambiguous, because each
    star is occupied by the ship(s) of exactly one player.)

The total number of pips in the game is 72, so every game of Antisocial Homeworlds
will end in a "score" summing to 72. If one player scores 35 pips, then the other
player must have scored 37 pips.

If the score is 36–36, that's a draw.


## Strategy tips

So far, I've played two games of Antisocial Homeworlds with my lovely wife,
and [one game with Trydnt on Homeworlds Live](https://homeworlds-live2.glitch.me/archive/view/225).
(That was my third game and Trydnt's first; and it shows. The final score was 42–30.)
All it takes to play Antisocial Homeworlds on HWL is an agreement
to follow the rules and for the appropriate player to resign when the stash is empty;
every Antisocial Homeworlds move is by definition a legal move in vanilla Homeworlds.

* The "Banker" setup (a small-medium home star) seems strong, because it allows you to move
    out immediately to large stars and soak up those pips for yourself.

* Spreading out to as many stars as possible, one ship per star, is the way to go.
    There is no benefit to keeping well-defended and/or color-balanced systems.

* In this variant there's no particular reason to keep a big ship at home. In fact, you'll probably
    want to move your big ship out, because a lone g3 at a distant star can be cashed in,
    but a lone g3 at home can't (because that would leave your homeworld vacant).

* Cashing in g3 to build three ships is a powerful move. If this game has an equivalent of
    the "[Gun Rule](https://web.archive.org/web/20101230055017/http://twoshort.net/)," it's that
    once your opponent has built a factory, you need to build one ASAP or you're gonna get
    trounced.

* Cashing in a y3 to soak up three high-pip pieces as stars can be an even more dramatic scoring move,
    but isn't "renewable" in the way a green factory can be. In fact, you may have to
    choose between soaking up that y3 as a star, or putting it back in the stash for your
    opponent to build immediately.

* Red ships and stars have no particular "use," other than scoring pips. You can think of red
    as "no power," or as "a power that doesn't apply in this particular game but we'll explain
    it later," depending on if you're using this game as a "stepping stone" to full Homeworlds.


## Solitaire challenge

Notice that because you're never allowed to move from one of "your" stars to one of "my" stars,
you might reasonably observe that our stars are never adjacent. Therefore the usual "shape of the galaxy"
is irrelevant: I don't need to lay out my stars as if I'm advancing toward your homeworld, because
in a very real mathematical sense I am _not!_ We might as well play side-by-side at the bar, all the ships pointing
in the same direction, with just a dividing line (or the stash placed between us)
to distinguish your half-galaxy from mine.

Your actions still affect me, of course, because the stash is shared; but Antisocial Homeworlds
is vastly more "[multiplayer solitaire](https://www.boardgamequest.com/top-10-multiplayer-solitaire-games/)"
than vanilla Homeworlds.

Now let's take it _completely_ solitaire! (Thanks to Trydnt for suggesting this idea.)

In Solitaire Antisocial Homeworlds, you follow the usual rules of Homeworlds, but without
any opponent. Set up your single home system with whatever binary star and ship you like, and then
start taking your turns. The game ends when the stash is empty, which of course means that
you've scored exactly 72 points. Congratulations! The challenge is: _End the game in as
few turns as you can._

The absolute mathematical minimum to empty the stash, if you were somehow able to
build-or-move-to two new pieces every turn, and given that you start with 3 pieces already
out, would be 17 turns. But in fact you'll build only one ship on your first turn;
and at some point you'll probably have to trade colors, netting zero points for that
turn. I don't know what the lowest _achievable_ number of turns is.

So far, my best is 21 turns ([spoiler walkthru](/blog/code/2021-04-04-antisocial-homeworlds.txt)).
Can you do better?

----

See [the discussion of this challenge on BoardGameGeek](https://boardgamegeek.com/thread/2634399/antisocial-homeworlds-solitaire-challenge).
