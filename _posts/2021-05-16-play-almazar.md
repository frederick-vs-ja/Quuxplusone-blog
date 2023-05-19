---
layout: post
title: "Play _The Search for Almazar_ online"
date: 2021-05-16 00:01:00 +0000
tags:
  adventure
  digital-antiquaria
  playable-games
  web
excerpt: |
  In [a previous post](/blog/2021/05/13/nifty-notes/#finally-there-s-two-pages-of-not),
  I explained how I'd become aware of Winston M. Llamas's circa-1981 text adventure game
  _The Search for Almazar_; googled up two different versions of its BASIC source code
  and gotten one of them running in a CP/M emulator; and put that source code
  and instructions on my GitHub ([github.com/Quuxplusone/Almazar](https://github.com/Quuxplusone/Almazar)).

  Three days later, I've produced a C99 port! You can compile it natively from
  [the C source](https://github.com/Quuxplusone/Advent/blob/master/Almazar/almazar.c),
  or — better — you can [play it online](https://quuxplusone.github.io/Advent/index-almazar.html).
  Let me know if you find any bugs!
---

Previously on this blog:
["Nifty notebooks, and _Almazar_"](/blog/2021/05/13/nifty-notes/#finally-there-s-two-pages-of-not) (2021-05-13).

In that previous post, I explained how I'd become aware of Winston M. Llamas's circa-1981 text adventure game
_The Search for Almazar_; googled up two different versions of its BASIC source code
and gotten one of them running in a CP/M emulator; and put that source code
and instructions on my GitHub ([github.com/Quuxplusone/Almazar](https://github.com/Quuxplusone/Almazar)).

Three days later, I've produced a C99 port! You can compile it natively from
[the C source](https://github.com/Quuxplusone/Advent/blob/master/Almazar/almazar.c),
or — better — you can [play it online](https://quuxplusone.github.io/Advent/index-almazar.html).
Let me know if you find any bugs!

This online version uses the same "C99 to Z-code to JavaScript" workflow that I used
back in 2013 to produce online versions of _Adventure_, _Adventure II_ (LUPI0440),
_Adventure 3_ (PLAT0550), and _Adventure 6_ (MCDO0551). This process deserves its own
blog post; but long story short, I compile the C99 code using Volker Barthelmann's
`vbcc` compiler attached to David Given's Z-machine backend; this produces Z-code.
Then I embed the [Parchment](https://github.com/curiousdannii/parchment) interpreter
(maintained by Dannii Willis, originally written by Atul Varma, based on Gnusto by
Tom Thurman...) to turn that Z-code into something you can interact with on a web page.
"It's giants all the way down."


## Deficiencies of the Osborne 1 version

The version I'd been playing in that CP/M emulator was the one published in
the SIG/M software library in 1983, credited to Bob Liddelow. (Liddelow seems to have been
affiliated with a user group called "Software Tools of Australia"; he's also the author
of books including _A Guide to Native Orchids of South Western Australia_ and _Wine for All_.)
As I played through it, I discovered three major issues with it:

First: There were typos! Three days ago I'd found four; now I'm up to eight. I'm not talking
little spelling mistaeks, either. The first bug I found was that whenever you tried to
`DROP` something, the game would crash with a `Syntax error`, because that particular codepath
misspelled the BASIC keyword `AND` as `ANR`. The next bug was that when you tried to `LIGHT LAMP`,
the game would reply "You are not carrying it" — unless you happened to be carrying the ring —
because that codepath said `10` (the ring) when it meant `IO` (the object of the current command).
The next two bugs were on the critical path: there are two actions you _must_ take in order
to reach the endgame, one after the other. The first of these actions would have no effect
because the magic number `79` had been misprinted as `7`; after fixing that bug, the second
action would still have no effect because verb number `20` had been misprinted as `29`. There were
also some bugs off the critical path, such as swapping `43` for `34` (breaking the verb `SWIM`);
writing `229` for `29` (breaking the verb `DRINK`); writing `EELSE` for `ELSE` (somehow _not_
a syntax error, but actually nerfing an entire puzzle); and incorrectly making `BURN` a synonym
for `KILL` instead of for `LIGHT`.

I can't satisfactorily explain the shape of these errors:
the first two seem like the kind of errors a modern OCR program would make, but never a human
(particularly, a human transcriber would never mistake `IO` for `10` in this context); but
the others (particularly swapping `43` for `34` and changing the meaning of `BURN`)
seem easy for a human and impossible for a computer.

Second: In order to fit the game onto the Osborne 1's 52-column screen, Liddelow abridged
some of Llamas's original text. Also, just like _Adventure_ and _Castlequest_, Llamas's original
_Almazar_ had both long and short room descriptions; Liddelow (maybe to conserve space on the Osborne,
maybe to fit on the SIG/M disk, maybe for aesthetics) removed the short descriptions. This massively
affects the "feel" of the game, and also loses at least one nice pun.

Third: Just as with _Castlequest_ and David Long's _Adventure_, the critical path for _Almazar_
leads through a combination-lock puzzle. The combination to this lock is revealed in one specific
in-game message. Bizarrely, Liddelow's version _changes the numbers in this message_ without
also changing the combination to the lock! So that's one more obstacle to victory (in addition
to the two or three bugs mentioned above).

So, it was really lucky that I also had the _80 Micro_ magazine version to compare against.

For my port, I did start with Liddelow's code, because I could dump it straight into a text editor
and start adding curly braces and stuff. But I took all the messages and room descriptions
(including the restored short descriptions!) from Llamas's _80 Micro_ version. Since both versions'
messages were full of typos (and the _80 Micro_ messages were even ALL CAPS), I felt free to
massage their grammar and spelling. One big liberty I took was to abbreviate many of the short room
descriptions in _Adventure_'s telegraphic style: Llamas's `YOU'RE IN A THREE WAY JUNCTION`
in my port becomes `You're at three-way junction`. So if you're a stickler for historical
fidelity, keep that in mind!


## Thoughts on "simulationism"

I've noticed that a lot of adventure games are affected by what I think of as "the pursuit
of perfect simulation." There will be some piece of the program that isn't really noticeable
by the player, but has dozens of lines of code devoted to meticulous simulation. Crowther's
dwarf-movement code is the ur-example of this. (Even though I love Crowther's dwarf-movement code!
See ["Observations on _Castlequest_'s code"](/blog/2021/03/21/castlequest-vs-adventure/#the-wandering-monsters) (2021-03-21).)
Any game with multiple containers (MCDO0551, for example) likely falls into this category, too.

_Almazar_'s simulationist urge seems to be expressed, remarkably, in its geography.
Even though Llamas picked a data representation in which
"leaving a room to the north does not guarantee entering the next from the south,"
his actual geography is almost exactly based on a square grid. You can count your steps west,
take a step north, count the same number of steps back east, and be sure that you're exactly
one step north of your starting point. Even in Llamas's three explicitly mazey sections —
the forest, the maze of twisty passages, and the diamond mine — there are no twists or turns;
the "tricks" are limited to making directions into no-ops at the edges of the maze, and reusing
room descriptions. I infer that the programmer took a bit of pride in his ability to make every
passageway connect up in the most absolutely logical and Euclidean way.


## Fun with gnomes

Llamas's wandering monster is semi-simulationist in a way that's new to me.
Whereas Crowther's dwarves wander realistically through the same geography as the player;
and Holtzman's gnome and werewolf (and Platt's dwarves) simply warp into the player's room
at random; Llamas's knife-throwing gnome tracks the player through a different geography.
(Serendipitously, this reminds me of a scene in [SCP-4739](http://www.scpwiki.com/introductory-antimemetics);
but that's probably the wrong mental image for this gnome!) This codepath runs on every turn
you spend inside the cave:

    7400 IF SC=0 THEN RETURN
    7410 IF OP(21)=0 THEN OP(21)=INT(RND*35)
    7420 IF OP(21)>RN THEN OP(21)=OP(21)-1
    7425 IF OP(21)<RN THEN OP(21)=OP(21)+1
    7430 IF OP(21)<>RN THEN RETURN

Visiting the gnome's headquarters sets `SC`, the "chased by gnome" flag. Every turn afterward that
you spend inside the cave, the gnome has a chance of spawning in a random room — possibly _outside_
the cave! Once the gnome has spawned, every turn you spend in the cave, the gnome
will move toward you along the number line. If the gnome has cornered you in the lighted room (room 15),
and you move into the tool room (16), the gnome will follow you, because those rooms' numbers differ by 1;
but if you move from the lighted room to the long corridor (17), you'll gain a one-turn lead and temporarily
evade the gnome.

The gnome is in most respects a normal object, but it's skipped by the code that prints object descriptions.
In other words, while you're outside the cave the gnome is both stationary and invisible. But it's still there!
Wander around outside the cave throwing the dagger at random, and you may eventually see
"You killed a nasty knife-throwing gnome!"


## Executive summary

I made a version of Winston Llamas's _Almazar_ you can play online.
[Play it here.](https://quuxplusone.github.io/Advent/play-almazar.html)
