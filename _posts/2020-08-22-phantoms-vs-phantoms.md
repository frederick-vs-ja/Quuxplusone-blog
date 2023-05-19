---
layout: post
title: 'Phantoms vs Phantoms (Geister)'
date: 2020-08-22 00:01:00 +0000
tags:
  board-games
  playable-games
  web
---

Circa June I got really into the board game
[_Phantoms vs Phantoms_](https://boardgamegeek.com/boardgame/2290/phantoms-vs-phantoms),
a.k.a. _Ghosts!_ or _Geister_.

It's basically [_Stratego_](https://boardgamegeek.com/boardgame/1917/stratego) with all of the cruft removed.
The games share the gimmick that the pieces have hidden identities — I know _where_ your pieces are but not what
_species_ of piece they are. I have to deduce your pieces' identities based on how you move them, which leads to
a nice bluffing aspect.

But in _Stratego_, the goal is to locate and then capture your opponent's flag; the board has 92 squares;
each player has 40 pieces of 12 species; some can move, some can't, and some have special movement rules;
moving your piece into an enemy piece might cause either or both of your pieces to be removed from the board,
and might reveal your own piece's identity to your opponent.
In _Phantoms vs Phantoms_, the goal is to reach one of two fixed exits; the board has only 32 squares;
each player has only 8 pieces of 2 species; all move the same way; and moving your piece into an enemy piece
simply captures it without revealing your own piece's identity.

The twist in _Phantoms_ is that each player has 4 "good ghosts" — colored blue on their hidden sides — and
4 "bad ghosts" — colored red on their hidden sides. Your goal is to move one of your blue ghosts to the exit;
naturally, your opponent will try to stop you. But _moving into a piece invariably captures it._ So you cannot
"blockade" the exit in _Phantoms_ the way you can blockade your flag with high-powered pieces in _Stratego_.
You move a piece in front of the goal? I take your piece and keep moving forward!

So, the only way to stop me pushing a blue piece to the exit is for you to capture all of my blue pieces.
If you capture all four of my blue pieces, you win.

Ah, but watch out: If you capture all four of my _red_ pieces, then _I_ win!

This makes for a quick tactical game that I find extremely pleasing. It's hard to tell with how little I've
played so far, but it seems like there's a rock-paper-scissors thing going on with the strategies.

- I ignore the hidden information and rush down the sideline with a blue "wide receiver"
    and one or two "blockers" running interference. I use plain old checkers tactics to capture
    any defenders who attempt to tackle my blue, and make it to the goal.

- You see me doing this game after game, and put your reds on defense. Now I can't capture defenders
    with impunity, because capturing four reds loses.

- I see you doing this game after game, and start watching which pieces run toward danger and which ones
    run away. I send red hit squads to take out your blues, stepping around pieces I think are your reds.
    But this is a slow process, giving you plenty of time to rush down the sideline with a blue...

----

I decided to implement _Phantoms vs Phantoms_ in JavaScript (just as I've implemented two more of my
favorite abstracts, [Homeworlds](https://github.com/Quuxplusone/Homeworlds)
and [Hanabi](https://github.com/Quuxplusone/Hanabi), in C++). I'm very bad at JavaScript, but I muddled
through by stealing a lot of code from "2048"; see
["Three variants on 2048"](/blog/2019/11/16/sqrt-2048/) (2019-11-16).

As of this writing, the only play mode is "human versus AI."
The current AI is based on Koki Suetsugu and Yusuke Orita's 2018 paper
["機械学習を用いないガイスターの行動アルゴリズム開発"](https://ipsj.ixsq.nii.ac.jp/ej/?action=repository_action_common_download&item_id=186127&item_no=1&attribute_id=1&file_no=1)
("Developing geister algorithms without machine learning").
Koki Suetsugu answered several questions for me via email; many thanks!
The AI is very easy to beat at the moment, but I'm hoping to make it more challenging in the future.

I should mention that _Phantoms_ shares another property with _Stratego_ — it has multiple printings
with different rulesets! In the physical edition I own, and in the version I programmed, your goal is to
move _into_ an exit square, and the board functionally has only 32 habitable squares. In the edition
targeted by Suetsugu's paper, all 36 squares are habitable, and your goal is to move _out of_ an exit square.

Play my JavaScript _Phantoms vs Phantoms_ online [here](https://quuxplusone.github.io/Ghosts/)!
