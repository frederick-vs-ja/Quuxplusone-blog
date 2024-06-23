---
layout: post
title: "_Evasive Maneuvers_ (1994)"
date: 2021-10-08 00:01:00 +0000
tags:
  digital-antiquaria
  playable-games
  web
---

I was recently thinking about one of the DOS shareware games
of my youth: _Evasive Maneuvers_ (Colin Buckley and Chris Blackwell, 1994).
It's a really polished and addictive VGA side-scroller in which you
pilot a little Imperial-Shuttle-ish spacecraft through obstacle-strewn
levels, trying to keep up with the implacable scrolling of the screen
(à la _Flappy Bird_ (2013)) while shooting and dropping bombs to eliminate
hazards and to keep your ship fueled by exploding rooftop fuel tanks.
It's a clone/homage to [_Scramble_](https://youtu.be/3Vc-RIkpk40) (Konami, 1981),
which I've personally never played but which has clearly spawned a lot of imitators.

![Photo credit: errorwear.com/evasive](/blog/images/2021-10-08-evasive-maneuvers.png){: .meme}

The game was programmed by Buckley and Blackwell's two-man operation
[Exaggerated Software](https://www.classicdosgames.com/company/exaggerated.html)
and released as shareware. The full game contains four levels.

I was all prepared to write about _Evasive Maneuvers_ as a "lost game."
See, for many years the full version _was_ lost, as far as the Internet
was concerned. The shareware level had very wide distribution, but the full game
didn't seem to exist anymore. In 2008, a member of the DosGames.com forums went
so far as to [write to Chris Blackwell](http://web.archive.org/web/20190304160451/http://www.dosgames.com/forum/about10416-0-asc-20.html)
requesting that the full game be re-released. Chris contacted Colin. Colin, having lost
the 1994-era .zip but still having the original source code, was able
to produce a new full version by recompiling from source!
Chris set up an official web page for the game,
[www.errorwear.com/evasive](http://web.archive.org/web/20100412204448/http://www.errorwear.com/evasive/),
which told the saga in greater detail and also provided a link to download
the reconstructed full game. _But,_ as of 2021, that official page's domain is defunct,
and while the Internet Archive's Wayback Machine managed to archive the text and images,
it failed to archive an actual copy of `evasive_full.zip`. In short: the full game
was lost, briefly resurrected, and then lost again...

...And found again! The Internet Archive's MS-DOS software library contains not only
a couple of copies of the shareware version, but also (as I learned only while doing my homework
for this blog post) [a copy of the full game](https://archive.org/details/msdos_Evasive_Maneuvers_1994).
The zip file's timestamps are from 1996, indicating that this is the
original full version, not Colin Buckley's recompiled version from 2008.

* Play the full _Evasive Maneuvers_ in your browser [on archive.org](https://archive.org/details/msdos_Evasive_Maneuvers_1994)

* Play the shareware level [on DosGames.org](https://dosgames.com/game/evasive-maneuvers/)

I found that the best experience came from downloading the .zip ([backup](/blog/code/Evasive_Maneuvers_1994.zip))
to play in a local DosBox. This lets you run `SETUP.EXE` to change the keybindings
for "shoot" and "drop bomb" away from "Ctrl" and "Alt" (e.g. to "space" and "X"),
before running the game proper with `EVASIVE.EXE`.

    brew install dosbox
    curl -L -O https://archive.org/download/msdos_Evasive_Maneuvers_1994/Evasive_Maneuvers_1994.zip
    unzip Evasive_Maneuvers_1994.zip
    cd Evasive
    dosbox -fullscreen -c "mount c $(pwd)" -c "c:"

So, this is _not_ another "lost game" blog post; _Evasive Maneuvers_ is still alive and well
after 27 years!
