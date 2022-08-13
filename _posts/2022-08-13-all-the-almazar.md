---
layout: post
title: "Playing _The Search for Almazar_ (1981)"
date: 2022-08-13 00:01:00 +0000
tags:
  adventure
  digital-antiquaria
excerpt: |
  Last year I ported Winston Llamas' _The Search for Almazar_ from BASIC to
  C for the Z-machine; see ["Play _The Search for Almazar_ online"](/blog/2021/05/16/play-almazar/) (2021-05-16).
  At the time I didn't blog a playthrough of the game; but now that it's
  been a year and my memories have faded somewhat, why not do
  the ["All the Adventures"](https://bluerenga.blog/all-the-adventures/) thing
  again?

  <b>This post contains major spoilers for all plot points and puzzles!</b>

  I'll be playing my own port of _Almazar_, which you can play online
  [here](https://quuxplusone.github.io/Advent/play-almazar.html).
---

Last year I ported Winston Llamas' _The Search for Almazar_ from BASIC to
C for the Z-machine; see ["Play _The Search for Almazar_ online"](/blog/2021/05/16/play-almazar/) (2021-05-16).
At the time I didn't blog a playthrough of the game; but now that it's
been a year and my memories have faded somewhat, why not do
the ["All the Adventures"](https://bluerenga.blog/all-the-adventures/) thing
again?

<b>This post contains major spoilers for all plot points and puzzles!</b>

I'll be playing my own port of _Almazar_, which you can play online
[here](https://quuxplusone.github.io/Advent/play-almazar.html).


## The early game

_Almazar_ doesn't ask if you want instructions; it just _gives_ them
to you.

          The Search For Almazar
                   Part I

    This program is the first of a projected series of programs
    whose central theme is the continuing search for the super being
    Almazar. This game, however, will play to a satisfactory ending
    if the player does not wish to continue the series.

The author apparently planned a whole series of games, although
I'm not aware that any sequels were ever made.

    The program accepts one- or two-word commands. Some examples:
      To take an object, type TAKE OBJECT, or T OBJECT for short.
      To go north, type NORTH, or N for short.
      To see what you're carrying, type INVE(NTORY).
      Type SCORE and the program will give you your current score.
      Type SAVE and the program will save the game for later play.
      Type QUIT and the game will be terminated.
      To light or turn off a lamp, type LIGHT LAMP or OFF LAMP.
      To get a description of the room, type LOOK.

    Other commands include SMASH, TOSS, SHOW, CROSS, etc.
    In addition, one may type ENTER to enter a shack or type LEAVE
    to leave a shack. Of course, if there is no way out you cannot
    leave. A hint: Caves are dark and often dangerous.

Okay, so the parser looks at only the first four letters of each
word. The instructions for `ENTER` and `LEAVE` are oddly specific;
and it's interesting that of the "other commands" only `SHOW` will
end up being important.
The instructions go on for three pages, and finally:

    Good luck, and may Almazar guide you to a safe journey.

    You are standing at the entrance to an old abandoned shack.
    To the west is a rocky path. A road goes north.
    > ENTER
    You are inside an old shack. There is a door to the west.
    There is an old oil lamp here.
    An old brass key is sitting here.
    > GET LAMP
    OK.
    > LIGHT LAMP
    You cannot light an empty oil lamp.

After getting the lamp and key, I head west.
The rocky path leads to a forest "maze," which is just
four rooms connected in a square. As I mentioned
[last year](/blog/2021/05/16/play-almazar/#thoughts-on-simulationism),
_Almazar_'s geography is almost entirely compatible with a square grid,
and Llamas seems to have taken some pride in fitting the rooms
into that grid, even though there was no _technical_ requirement
to do so.

Following the road northward leads to three side trails. On the
east side of the road we find a murky pool:

    You are on the edge of a murky pool. A dark liquid is floating
    on top of the pool. There is a small path from the north.
    A small walk leads west.

and an abandoned mine:

    You are at an entrance to an old abandoned mine. A small path
    leads south. A large path leads west.
    > EAST
    There is insufficient light to see by.

Between the pool and the mine is a bit of scenery with a greeting
from Almazar himself, and a crystal ball that seems to be a red herring:

    > NORTH
    You are standing by a large oak tree. Engraved on the bark is
    a message. Paths lead north and south.
    There is a large crystal ball on the ground.
    > READ MESSAGE
    "The great Almazar bids you well. Though you will encounter
    many trials, he shall provide for you. He that is both water
    and flame shall send you a gift to aid you in your quest. Thus
    saith Almazar: 'Live, and you shall live.'"
    > GET BALL
    It's too heavy for you to take.
    > KICK BALL
    Ouch! Every bone in your foot just broke.

Given the existence of our oil lamp, we intuit correctly that the
dark liquid in the pool (both water and flame?) is `OIL`.
This plus the box of matches we found in the forest lets us
`LIGHT LAMP` and explore further:

    > LOOK
    You are inside a mine. There are passages in every direction.

The mine, like the forest, is a perfect grid (this time
2x3 instead of 2x2). The trick that makes it feel bigger,
until you start dropping items to map it, is that when
you try to go off the edge you get placed back into the
same room again, instead of the usual
"There is no way to go in that direction." In other words,
the mine is bounded by [invisible walls](https://en.wikipedia.org/wiki/Invisible_wall).
One room away from the entrance, we find what I assume is our
first treasure: a diamond! (Or are we trying to locate
the super being known as Almazar? I'm still a bit confused
on that count.)

On the west side of the road we find a little cave.
As in [_Castlequest_](/blog/2021/03/19/all-the-adcastlequest-part-1/),
this is one of those caves with "paths" and "trails"
rather than passages; but the descriptions are quite
evocative and remind me favorably of David Long's
additions in [MCDO0551](/blog/2019/01/28/mcdo0551-sandbox-game/).

    You are at an entrance to a small cave. A small narrow hole is
    west. A passage leads east. On top of the hole a sign reads:
    "Beware, brave adventurer. For it is the small things in life
    that so often destroy it."
    > WEST
    You are inside a cave. Light filters through from the east.
    A small path leads west. A smaller path leads south.
    > WEST
    You are in an extremely narrow part of the cave. Exits lead
    east and west.
    > WEST
    You have come to a place where three paths meet. A small path
    leads north. Larger paths lead east and west.
    > WEST
    You are in a long hall. Engraved on the rocks is a message.
    Exits lead east and west.
    > READ MESSAGE
    "Merlin was here."
    > WEST
    A tall wall rises above you. You are nearly encircled by large
    unclimbable canyons. The only exits lead south and east.
    There's an empty bottle here.
    > SOUTH
    You are in a little maze of twisty passages.

...Great.

This maze is easy to map by dropping
items. It turns out to be _almost_ a 3x2 grid
with "invisible walls" at the edges just like
the abandoned mine; but the room that should
be in the middle of the north edge has been
disconnected from the grid and turned into
a dead end, where we find a black rod.

> Incidentally, [John P. Wesson's map](/blog/images/RPI-Almazar-Notes.pdf)
> shows an additional "room 21" in the middle
> of the north edge, but I don't see such a
> room in either the _80 Micro_ or the Liddelow
> version.

As in _Adventure_, we can wave the rod to bridge a chasm, which
we find north of the cave's main axis:

    A broad chasm separates you from a ledge to the north. A small
    path leads south.
    > WAVE ROD
    A crystal bridge now spans the chasm.
    > NORTH
    You are on a small narrow ledge. There is a cavern across the
    chasm to the south. A walk leads down.
    There are rare spices sitting on the ground.
    > GET SPICES
    You have no container to put it in.

We need the bottle from tall wall canyon in order to collect the spices.
Down from the ledge we find the headquarters of the gnomes who are to
this cave as nasty little dwarves are to Colossal Cave; we collect a
jeweled dagger from that area; and then (in the game's most egregious
departure from grid geography) we find ourselves approaching the cave's
main axis from the south again!

Continuing north along the road, the next spur goes east:

    You have climbed up a hill. The view is spectacular, as one
    would expect. Sharp rocks protrude from the base of the hill.
    Beyond a river, a strange house can be seen. The silence here
    is uncanny. Paths lead east and west.

    A loud voice emanates from beyond, saying: "Time grows short,
    mortal, before we feast again."
    > EAST
    The gravel path ends here. A large rock marks this spot.
    There's an ancient-looking staff lying here.

And soon we reach the river, guarded by an ominous
sign reading "Fish from the Amazon." Besides the diamond,
bottle of spices, and jeweled dagger,
our available inventory at this point consists of the
lamp, matches, black rod, key, and staff.

    You are at the river's southern edge. There is a small sign on
    the shore. A road goes south.
    > NORTH
    You cannot cross the river.
    > WAVE ROD
    Nothing happens.
    > SWIM
    I don't understand.
    > JUMP
    You did not jump far enough. You fell into the river.
    A school of piranhas devours you.
    Do you want another chance?

Each death costs you ten points, by the way.

The actual way across the river is to channel your inner
Charlton Heston:

    > WAVE STAFF
    Holy Moses! The river just split in two! A dry path leads to
    the other side.

You can then explore the area north of the river for about seven
turns, until...

    You incompetent fool! You forgot to close the river and as a
    result a flood has overrun the countryside.
    Do you want another chance?

Right, I guess closing up the water after you is also part of that story.


## Beyond the river

Beyond the river, the map opens out into a target-rich
environment of puzzles.

    You are at a crossroads. A road goes north and south.
    Exits lead east and west.
    > NORTH
    You are standing by a large gargoyle statue. It seems to be
    staring at you. Inscribed at its base is the number 13.
    A road goes south. Exits lead east and west. A house is north.

West of the crossroads we find a "worthless-looking ring,"
an outhouse with the message "[Frodo lives!](https://en.wikipedia.org/wiki/Frodo_Lives!)",
and a stone idol with an emerald eye.

    A large stone idol stands in front of you. There is a small
    charred pit in front of the idol. A path leads east.
    An emerald eye sits on top of the idol.
    > GET EYE
    The god is incensed by your audacity. With a blink of an eye
    you fall dead to the ground.
    Do you want another chance?

East of the crossroads we find a bale of hay, a garden full of
carrots that seem to be just window dressing, and a golden apple
guarded by a snake.

North of the crossroads we enter the "odd-looking house,"
which contains another ten or so rooms. The second room of the
house contains a "priceless figurine," and also what I take to
be yet another _Adventure_ homage:

    You are in the reception hall. A large table is on one side of
    the room. As you gaze upon the table you see someone stare back
    at you. A can of Pledge sits on the far side of the room. Stairs
    lead up. The hall extends south.

(As in _Adventure_, you're looking at your own reflection.)

In the kitchen we find some incense, which we can `BURN` to appease the idol;
we also find some cheese, which we can feed to the mouse
in the attic, which we can feed to the snake in the orchard.

    You're in an orchard.
    A giant snake is jealously guarding a golden apple.
    > FEED SNAKE
    The mouse was used for carcinogenic testing. The snake is dead.

At this point our treasure list is: diamond, bottled spices,
jeweled dagger, priceless figurine, golden apple, emerald eye,
and a string of pearls we found in the upstairs bedroom.
Across from the bedroom is the library:

    You are in the library. A large heavy book lies open on top of
    a desk by a window. A door is west.
    The window is open.
    > READ BOOK
    A small axe flies through the window and strikes you dead.
    Do you want another chance?

You've got to be kidding me!

    > CLOSE WINDOW
    OK.
    > READ BOOK
    "And so it came to pass that in the third day of the seventh
    month of the twenty-first year a great event happened. The earth
    opened and took within it the fruits of the people's labor and
    gave upon them a greater glory."

Finally, in the living room we find "a small statue with a round head";
`TURN HEAD` reveals a secret passage to the north! (This is the second
and last major deviation from _Almazar_'s grid geography.)

    > NORTH
    You are in a small hidden room. A rope hangs from the ceiling.
    There is an open panel to the south.
    > UP
    There is no way to go in that direction.
    > CLIMB
    I don't understand.
    > CLIMB ROPE
    I don't understand.
    > PULL ROPE
    A trap door opens beneath you. The fall knocks you unconscious.
    When you awaken you find that...
    You are in the basement. A corridor leads north.
    An angry wizard wearing a platinum chain is in the room with you!

Yipes!

    > GET CHAIN
    The wizard won't let you.
    > NORTH
    The wizard blocks your way.
    > XYZZY
    I don't understand.
    > WAVE ROD
    Nothing happens.
    > WAVE STAFF
    Nothing happens.
    > THROW DAGGER
    OK.
    > KILL WIZARD
    With what, your bare hands?
    > YES
    The wizard raises his hand and in an instant you find yourself
    uncomfortably hot. As you look up into the sky you see a large
    stone idol. You are being offered as a sacrifice.

    You have died three times. The game is therefore suspended.
    You have scored -9 points.
    You obviously have no idea what's happening in this game.

Start over, and this time it's obvious that the solution is Frodo's ring:

    > SHOW RING
    The ring glows brightly. A lightning bolt strikes the wizard!

Finally, we find the place to deposit all these treasures we've
been accumulating:

    A large combination vault is standing in front of you. A sign on
    top of the vault says, "Deposit treasures inside the vault for
    full credit." A corridor leads south. A door is east.
    > NORTH
    You have to open the vault to get inside it.
    > OPEN VAULT
    The vault is locked.
    > UNLOCK VAULT

The combination is the three numbers from that unusually deadly book
we read in the library: 3–7–21.

> Incidentally, Bob Liddelow's version of _Almazar_, distributed in
> SIG/M Volume 142, truncated and altered the book's text so that this
> puzzle became unsolvable! Liddelow's book says merely: "It came
> to pass on the 23rd day of the 37th year that the Earth opened
> for the glory of the people who had laboured." But he didn't
> change the combination.

Several trips later:

    > NORTH
    You are inside vault.
    A large diamond is lying here!
    A platinum chain is lying here!
    There's a bottle of rare spices lying here!
    There is a jewel-encrusted dagger here!
    > INVENTORY
    You are carrying:
        Emerald eye
        String of pearls
        Priceless figurine
        Oil lamp
        Kraft cheese
        Matches
    > DROP EYE
    OK.
    > DROP PEARLS
    OK.
    > DROP FIGURINE
    The delicate figurine breaks upon hitting the ground.

Darn it! Yet another _Adventure_ homage! No, there was no hint
that the figurine might have been delicate or fragile before it
was dropped. I guess now I know what to do with that bale of hay.

Finally, we've collected all eight treasures, and there's nothing
to do but return to that out-of-place outhouse.

    You are inside the outhouse. A strange message is pasted on the
    wall. A doorway is south.
    > READ MESSAGE
    "Frodo lives!"
    > DOWN
    There is no way to go in that direction.
    > UP
    There is no way to go in that direction.
    > SIT
    I don't understand.
    > SHIT
    I don't understand.
    > SHOW RING
    You're not carrying it.

Hmm. Go back and get the ring; and this time:

    > READ MESSAGE
    In a blaze of glory you find yourself in a land far away!
    You have scored 90 points.
    All honor thee, Grandmaster Adventurer!


## Final thoughts

_Almazar_ is a nice little game. Not nearly as large as
_Adventure_ or _Castlequest_, but with neat little puzzles
that make sense. Almost _too_ much sense: admittedly I was
spoiled already, but there were no particularly evil puzzles
here. Even the non-sequitur endgame was reasonably well
signposted. The small size of the game is helpful, given
that I count at least ten distinct ways to die!

The only confusing thing about it is... what ever happened
to "the super being Almazar"? We get mysterious voices from beyond,
stone idols, and vaguely biblical wordings right up to the
last minute; but ultimately the game turns out to be a treasure
hunt with a Tolkienesque wizard/ring endgame. Of course
maybe all my questions would have been answered had we ever
received _The Search for Almazar, Part II_!

----

Previously on this blog:

* ["Nifty notebooks, and _Almazar_"](/blog/2021/05/13/nifty-notes/) (2021-05-13)
* ["Play _The Search for Almazar_ online"](/blog/2021/05/16/play-almazar/) (2021-05-16)
