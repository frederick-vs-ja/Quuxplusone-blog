---
layout: post
title: "An interview with Arthur O'Dwyer"
date: 2021-05-11 00:01:00 +0000
tags:
  adventure
  castlequest
  digital-antiquaria
  help-wanted
---

You might have noticed in your news feed recently a piece by David Cassel for
_TheNewStack.io_, titled
["Software Developer Tracks Down Code for a Beloved 41-Year-Old Text Adventure"](https://thenewstack.io/software-developer-tracks-down-code-for-a-beloved-41-year-old-text-adventure/)
(2021-04-25). The piece was based on "an email interview," which is a bit of
journalistic newspeak that means "David sent me an email 36 hours before deadline; I responded."
([Not that there's anything wrong with that!](https://www.youtube.com/watch?v=rGAyQAkXajg))
Anyway, it's a good piece of promotion. But I did tell David at the time that since
I'd gone to the trouble of writing up a long-form reply, I'd probably put it up on my blog
at some point... And now here it is.

_Arthur O'Dwyer and Castlequest: The Long-Form Email Interview._

----

<b>What is it about _Adventure_ that's inspiring so much dedicated research?</b>

Well, both of my parents were computer programmers (they both worked for General Electric Information Systems),
so that got me into computers at a very early age. And it also meant that we had Internet in the house
(via the Maryland Public Libraries' [SAILOR](https://www.sailor.lib.md.us/about/) system),
which made it possible to get to GEnie.
(I was born 1984, and we're probably talking 1995-ish for GEnie and so on.)
And, my parents' having done computer stuff at university in the 1970s meant that they were both exposed to _Adventure_ already.

I don't know when or where she made it, but my mom actually had a deck of hand-written index cards
holding the entire map of 350-point _Adventure_: each card had the name of a room, and then a list of
exits cross-referencing other cards. So if you had this deck of index cards, you could use it to plan
out a route through the cave before turning on the computer; or you could act as the computer for someone else,
and let them "move around" in the cave just as well as they could on the computer.

(Circa middle school, I got into the [Adventure Game Toolkit](https://en.wikipedia.org/wiki/Adventure_Game_Toolkit),
and made a couple of text adventures with that; and I did a few times actually print out the code and carry it on
Boy Scout camping trips and the like so that my friends could play the game while I acted as the computer.
Also related: [_Parsely_](https://www.memento-mori.com/).)

Anyway, I wouldn't say that I lived in an _Adventure_-themed house or anything, but I definitely had more
incidental exposure to text adventures than the average '90s kid. And, I suppose, an early exposure to the idea that
text adventures can have a physical paper component!

(Although of course Infocom was famous for their [feelies](https://futuryst.blogspot.com/2009/02/thoughts-about-feelies.html).
I arrived too late to really have any exposure to Infocom;
I picked up a battered copy of [_HHGTTG_](https://gallery.guetech.org/hhgttg/hhgttg.html)
at a yard sale circa high school, but that was all.)

My latter-day _Adventure_ archaeology started circa Thanksgiving 2010, I think.
(I would have guessed earlier, but I know that Looney Labs'
[_Back to the Future_ card game](https://www.looneylabs.com/games/back-future-card-game)
had just come out, which pegs it to 2010.)  I realized that _Colossal Cave_ would make a great setting
for a _Back to the Future_–style card game. So that led to
[_Colossal Cave: The Board Game_](https://boardgamegeek.com/boardgame/121751/colossal-cave-board-game)
(and thus my Twitter handle). Having grown up amidst _Adventure_ variants, I didn't feel like there'd be
any legal problems with a card game explicitly based on _Adventure_ (and there never were),
but as the Kickstarter began feeling really real, I started to feel like I ought to get buy-in
from the original writers, anyway. (And from Andy Looney, since I was also ripping off
[_Fluxx_](https://www.looneylabs.com/games/fluxx).)
So I ended up contacting both Don Woods (of Crowther & Woods) and Dave Platt (550-point _Adventure_),
both of whom were very nice people.
[I even got to visit Don Woods for a game night!](https://www.kickstarter.com/projects/765522088/colossal-cave-the-board-game/posts/213205)
Will Crowther, on the other hand, guards his privacy well; Woods knew how to contact him,
but felt that "random guy wants to make a board game" didn't qualify as a good reason to disturb him,
and said go right ahead.

![Will Crowther playing _CC:TBG_, 2016-02-18](/blog/images/2021-05-11-will-crowther.jpg){: .float-right}

Much later, I got an email from Will Crowther's daughter Sandy saying that they'd gotten him
the board game for Christmas 2015 and that he'd enjoyed playing it. :)

So _CC:TBG_ got me into "nosily contacting people from that era"; and it also got me into "resurrecting games in new media."

For the Kickstarter, I decided that it would be cool to have a version of _Adventure_ you could play online.
Of course plenty such already existed; but they tended to be based on
[Graham Nelson's Inform port](https://iplayif.com/?story=http://www.ifarchive.org/if-archive/games/zcode/Advent.z5),
which was a very modern, Infocom-style, legalistic take on the original.
You couldn't just say `OPEN` while holding the keys; you had to say `UNLOCK GRATE WITH KEYS` and then `OPEN GRATE`.
You couldn't just say `HOUSE` to get from the grate back to the well house; you had to navigate there with compass directions.
So I puzzled out how to make something more authentic to the original (standing on the shoulders of Donald Knuth and David Given),
and ended up with [a C version playable in Parchment](https://quuxplusone.github.io/Advent/).
Every _CC:TBG_ backer received a custom-printed USB stick with a copy of the game (both the [350-point](https://quuxplusone.github.io/Advent/play.html)
and [550-point](https://quuxplusone.github.io/Advent/play-550.html) versions).

So that got me into "porting _Adventure_ variants into C," which meant "learning what _Adventure_ variants existed."
Which led me to learn of David Long's 751-point _New Adventure_ — which is still lost.  I'd never played that version,
or even heard of it, but it was one of the better-merchandised versions, copiously documented and advertised.
Above my computer on the wall right now is [the Dennis Donovan poster](http://www.club.cc.cmu.edu/~ajo/in-search-of-LONG0751/readme.html#poster)
I got from someone on eBay in 2017 — I still look at it every day!
So I made that ["In Search of LONG0751"](http://www.club.cc.cmu.edu/~ajo/in-search-of-LONG0751/readme.html) web page,
and then at the bottom of the page I listed some other games — ones I'd actually played as a kid — that seemed
to be lost as well: _Castlequest_ and _Black Dragon_. ...And that's where _Castlequest_ entered the picture!

From there, [my previously blogged timeline](/blog/2021/03/09/castlequest/#timeline) can pick up the story.

<b>How would you describe your final reaction after successfully discovering (and then exploring) all of the game's original code?</b>

I was thrilled to find that the copyright deposit contained the actual source code.
None of us (me, Kershenblatt, or Holtzman) actually expected the deposit to contain a playable version of the game;
we all expected a map, walkthru, external help file, or maybe even less.
As I said on my blog, I was also surprised (ruefully, or maybe not so surprised in hindsight)
to find that not only had I never beaten the game as a kid, I'd never even seen two-thirds of it!
This probably did help it stick in my mind, though. I'm sure I never got far in _Black Dragon_, either.

(Also in the late 1990s, I got only about a quarter of the way into [_Hexx: Heresy of the Wizard_](https://www.youtube.com/watch?v=jTuYqQlNPgQ),
which made it stick in my brain forever as well. I occasionally have dreams about _Hexx_. But it's not a "lost game."
A couple of years ago I opened it up in DosBox and beat it as thoroughly as I've now beaten _Castlequest_ —
with only the tiniest modicum of hex-editing to teleport past a particularly tricky level.)
By the way, _Hexx_ and the save-editing tool I used are [on my GitHub](https://github.com/Quuxplusone/Hexx).)

<b>Wikipedia says GEnie didn't launch until 1985; so, where was the game during the preceding four years?</b>

It was running on a computer system at RPI. Mike Holtzman wrote to me in 2016:

> I wrote CastleQuest with my roommate, Mark Kershenblatt, at RPI in the late 70’s,
> where it was available on the school’s timesharing system. [...] On graduation I
> bequeathed the game to the RPI chapter of the ACM, who kept it running on the
> school’s computer system.
>
> Sometime later, Bob Maples visited the school and saw the game. At the time he worked
> at The Source. He contacted us and offered to port the game to The Source’s timesharing system
> and split the royalties with us. Later he did the same at Genie and CompuServe. Apparently
> he never divulged the fact that he was not actually the author of the game, only the ‘publisher’."

Bob Maples was also the publisher (and [maybe author](https://archive.org/details/Online_Today_Vol_08_02_1989_Feb/page/n23/mode/1up))
of _Black Dragon_. My internet-sleuthing turned up evidence
that Maples had died in April 2001, although of course I'm not 100% sure of that. (If I recall correctly,
it was one of those online "memorial/guestbook" pages. It's now a 404, with
[nothing in the Wayback Machine](http://web.archive.org/web/*/http://grave-records.mooseroots.com/l/3276408/Robert-Henry-Maples-MS).)

<b>Do you have any idea what you'll be investigating next?</b>

Never. ;)

I'd appreciate it if you could find a way to name-drop _Black Dragon_ in your piece — as another lost game
from GEnie and Bob Maples — since maybe that'll eventually lead to someone finding it.  As you've seen,
sometimes these things percolate for 10 years before the right pieces come together, so every little bit
of getting-the-word-out helps.

Jason Dyer points out that _The Pits_ is another lost adventure game
[with a USCO entry](https://cocatalog.loc.gov/cgi-bin/Pwebrecon.cgi?v1=1&Search_Arg=TXu000039634&Search_Code=REGS&CNT=10&SID=1).
So if Jim Walters or Dave Broadhurst is out there, they should know there are a lot of people
who'd like to see that game resurrected, too!  Personally, I never played nor even heard of _The Pits_;
but I've heard of it as an adult in the sense that there are definitely people out there looking for it.

----

After David Cassel's piece was published, I was invited to give a lightning talk on _Castlequest_ at C++Now;
watch it [here](https://www.youtube.com/watch?v=-4LN7Uy46-0).
In that talk, I gave a few more examples of "lost games" that could use some press:

- _Dor Sageth_, a sci-fi text adventure from GEnie

- Jason Dyer's [list of lost mainframe games](https://bluerenga.blog/2015/03/19/lost-mainframe-games/),
    including Peter Langston's _Wander_ (1974), Olli J. Paavola's _LORD_ (1981), John Sobotik and Richard Beigel's _FisK_ (1980),
    and Gary Kleppe's _Underground_ (1978).

- The _Adventureland_ archive lists a couple of
    [adventure games from the University of Waterloo](http://www.lysator.liu.se/adventure/Mainframe_adventures.html#New_Adventure):
    Mark Niemiec's _New Adventure_; Brad Templeton and Kieran Carroll's _Martian Adventure_.

- O.G. _Adventure_ taxonomist Russel Dalenberg posted a [list of lost _Adventure_ variants](http://www.club.cc.cmu.edu/~ajo/in-search-of-LONG0751/2009-02-02-lost-versions-of-adventure.txt)
    circa 2009, including a 700-pointer by Denny Koch, Don Fry, Keith Moe, Lou Mackey, Greg Markow, and John Logan;
    a 535-point version featuring a piece of Venetian glass as a treasure; a 365-point version created by
    [Lauren Weinstein](https://www.vortex.com/lauren) while he was at UCLA (but now lost, says Weinstein,
    says Dalenberg); a 375(?)-pointer from Colorado State University featuring a witch's gingerbread cottage.

If you know anything about any of these games, drop me a line!
