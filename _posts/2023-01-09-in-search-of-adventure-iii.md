---
layout: post
title: "In search of _Adventure ]I[_ (1981-ish)"
date: 2023-01-09 00:01:00 +0000
tags:
  digital-antiquaria
  help-wanted
---

As readers of this blog may know, I'm interested in digging up old text adventures
and other games that have been "lost." My one success story so far is [_Castlequest_](/blog/2021/03/09/castlequest/) (1980);
I'm still searching for David Long's _Adventure_ ([LONG0751](http://www.club.cc.cmu.edu/~ajo/in-search-of-LONG0751/readme.html)),
Bob Maples' _Blackdragon_, [_Dor Sageth_](http://ascii.textfiles.com/archives/2923),
[_The PITS_](http://web.archive.org/web/20140528184628/http://games.wwco.com/pits/),
and anything on [this list](http://www.club.cc.cmu.edu/~ajo/in-search-of-LONG0751/2009-02-02-lost-versions-of-adventure.txt).
(If you have any information on any of these, please email me!)

In late November 2022, Rick Hammerstone wrote to tell me about a text adventure he'd played at
a computer summer camp in Fairfax County, Virginia, circa 1981 or 1982. The game was called "Adventure ]I["
(that is, "Adventure III" with typographical influence from 1977's <a href="https://en.wikipedia.org/wiki/Apple_II">Apple ][</a>),
but it was taxonomically unrelated to Dave Platt's 550-point [_Adventure 3_](https://quuxplusone.github.io/Advent/index-550.html) —
in fact it's not a variant of the Crowther & Woods _Adventure_ at all; it's its own thing.

The game claimed to have a maximum score of 1400 points. It had no wandering monsters, but
it did have a "nuclear reactor" which would melt down at turn 240 unless you solved its puzzle
first; a mysterious "Zarka" which could be enraged, combed, or fed pizza; a "Frobozz Automatic
Coupler" that performed marriages; quicksand; a clown; a ski lift; a waterfall you could swim under;
and much more.

Have you ever played this game? Do you have any materials related to it? Are you the author of it?
If the answer to any of these question is "yes," please email me and let me know!


## We have the engine

Rick actually has a copy of the game's engine, which is written in HP Time-Shared BASIC:
he printed it out back in 1981-or-1982! But he lacks printouts of the data files containing
the descriptions of the rooms and objects. Without those data files, not only do we lack the
descriptions of any of the rooms or objects, but also the _names_ of most of the objects in
the game; the number of points you score for each treasure; and so on.

A few of the treasures' names show up in the engine: there's a vase that shatters just like in _Adventure_,
a "sparkling emerald wedding ring," a "beautiful jade statue,"
and a chest containing a "dazzling nugget of gold." But that's pretty much all we can deduce
without the data files.

### Bug bounty program

I've put a transcript of Rick's game-engine files [on my GitHub here](https://github.com/Quuxplusone/Advent/tree/anon1400/ANON1400).
I probably introduced some mistakes during the manual transcription process.
Therefore I’m offering a "bug bounty" of $5 per error to anyone who reports mistakes in the
transcription. Note that my goal is fidelity to the original, so "mistakes" in the original
(e.g. the word `Comand` on page 1) do not count. Vice versa, if you point out where my fingers
accidentally _corrected_ the spelling of a word that was misspelled in the original, you get a bounty!
Submit reports to me via email, or as GitHub pull requests.
