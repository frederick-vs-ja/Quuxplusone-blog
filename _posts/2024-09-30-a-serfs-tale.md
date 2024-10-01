---
layout: post
title: "Playing _A Serf's Tale_ (1986)"
date: 2024-09-30 00:01:00 +0000
tags:
  adventure
  playable-games
excerpt: |
  The other week I was contacted by a fellow digital antiquarian re an erratum
  in [the "Adventure Family Tree"](https://mipmip.org/advfamily/advfamily.html)
  (managed by Nathanael Culver and hosted by Mike Arnautov). That page had listed
  under "No known download":

  > `BROO_XXX`: "Enhanced" version of `WOOD0350`, with "added locations, text added, puzzles reworked," by Nigel Brooks.
  > [...] This version was later renamed "A Serf's Tale, A Retelling of the Original Adventure".

  In fact, wrote our correspondent, _A Serf's Tale_ is very much alive and well;
  the ZX Spectrum tape image survives, and is playable in any Spectrum emulator,
  such as [in JSSpeccy on spectrumcomputing.co.uk](https://spectrumcomputing.co.uk/entry/6955/ZX-Spectrum/The_Serfs_Tale/).
  Ctrl+F "speccy" on that page, or (modulo link-rot) [click here](https://spectrumcomputing.co.uk/playonline.php?eml=1&downid=42595),
  to play _A Serf's Tale_ online.

  So I spent a few days trying to beat _A Serf's Tale_ — and eventually succeeded,
  with the help of several vintage walkthroughs and hint files.
  Spoilers galore below the break.
---

The other week I was contacted by a fellow digital antiquarian re an erratum
in [the "Adventure Family Tree"](https://mipmip.org/advfamily/advfamily.html)
(managed by Nathanael Culver and hosted by Mike Arnautov). That page had listed
under "No known download":

> `BROO_XXX`: "Enhanced" version of `WOOD0350`, with "added locations, text added, puzzles reworked," by Nigel Brooks.
> [...] This version was later renamed "A Serf's Tale, A Retelling of the Original Adventure".

In fact, wrote our correspondent, _A Serf's Tale_ is very much alive and well;
the ZX Spectrum tape image survives, and is playable in any Spectrum emulator,
such as [in JSSpeccy on spectrumcomputing.co.uk](https://spectrumcomputing.co.uk/entry/6955/ZX-Spectrum/The_Serfs_Tale/).
Ctrl+F "speccy" on that page, or (modulo link-rot) [click here](https://spectrumcomputing.co.uk/playonline.php?eml=1&downid=42595),
to play _A Serf's Tale_ online.

So I spent a few days trying to beat _A Serf's Tale_ — and eventually succeeded,
with the help of several vintage walkthroughs and hint files.
Spoilers galore below the break.

----

_A Serf's Tale_ was written by Nigel Brooks and Said Hassan, publishing under the name "Smart Egg Software"
(although some in-game messages still refer to their old name, "Adventure Software").
_AST_ is basically WOOD0350, but with a few very interesting additions — and two very annoying ones!

The back of the game box explains the game's title, as well as filling in
some backstory that I'd normally expect to get from the in-game help text.

> You're a lowly farmhand who unearths the mildewy journal of
> a past adventurer. Tempted by visions of wealth and fame,
> you set out across the Empire into the wilderness! [...]
>
> You set out on your quest and travel for many uneventful weeks
> before arriving at a lonely Waystation upon the dusty road.
> This marks the boundary of the wilderness. Here, you reprovision
> yourself using the last of your coins, staying one night to chat
> with the friendly landlord, and then press on across the moor.
> It is possible that you were overheard while discussing your map,
> for two rough-looking characters leave shortly after you,
> tracking you to a lonely spot where they rob you of everything...

This backstory leads us to _AST_'s first annoying change versus _Adventure_:

## The unskippable cutscenes

    You are standing in a clearing beside a small brick building,
    surrounded by trees. A dusty road leads north. Between the
    trees the hazy sunlight captures swirling motes of dust and
    busying insects in its rays. A little stream flows out of the
    building and south down a gully.

    What now? IN

    Try a compass direction.

    What now? E

    You are inside the well house. Cool, clear spring-water rises
    from a rocky trough in the floor and flows through an opening at
    the base of the south wall.

    A rectangle of light stretches from the doorway, illuminating
    some steps set against the far wall beneath a trapdoor to the
    ceiling.

    What now? INVENTORY

    You are carrying:-
    a pair of leather breeches (worn)
    a leather jerkin (worn)

What's this? No lamp or keys! To gain access to the cave,
you have to visit two _very_ out-of-the-way places: the southeast
and northwest corners of the map. This is not clued anywhere that I'm
aware of; there's no logical "puzzle" to solve here; with no possessions you
simply have no way to get past the locked grate, and the solution is to map
the entire above-ground area (including a Witt's End–style marsh that
kills you after a certain number of turns, and at least two distinct rooms
with identical descriptions) until by going
`N`, `E`, `E`, `E`, `SE`, `SE`, `SE` you find yourself dropped into a cutscene:

    What now? SE

    You cross a little gully, climb a hillside and find yourself in
    a vast, park-like area of the forest, filled with great,
    thick-stemmed, majestic trees.
    [more]
    Then suddenly, you hear angry shouts, and the sounds of sword-
    fighting. There is an agonized cry, and then... silence.

    You creep slowly forward. Between the trees you find a
    man, dead, run through, his shirt bloody!
    [more]
    Clutched in his hand is a part of your map... he was one of
    your assailants! You take the map, and in doing so disturb the
    body. Hidden beneath his arm are your keys. Stuffing these into a
    pocket you start off through the trees, running until your heart
    pounds and your legs are aching.
    [more]
    You are in a pleasant young wood beside a valley.

    What now? INVENTORY

    You are carrying:-
    a pair of leather breeches (worn)
    a leather jerkin (worn)

    and in your pockets:-
    a set of keys
    a torn corner of a map

This demonstrates another interesting feature of _Serf's Tale_:
your inventory is divided into your "hands" and your "pockets."
Items in your pockets can't be used, although that's not always obvious:

    What now? READ MAP

    You can see:
    a torn corner of a map

    What now? RETRIEVE MAP

    OK.

    What now? READ MAP

    From the remaining scrap you can just trace the route north-west
    over the moor to the Waystation.

The northwest corner of the aboveground map is normally an invisible wall:

    Interminable, granite-flecked moorland rolls unbroken to the
    farthest horizon.

    What now? NW

    The land around you seems bleaker and wilder as you trek
    across huge slopes dotted with giant boulders.
    [more]
    After a while you discover that you have wandered in a great
    circle and are back at the roadside.

But if we've read the map (and are still holding it), then we can successfully
pass through that wall:

    Guided by the map, you find your way across the moor and back to
    the Waystation.

    The Patron greets you and listens with sympathy to your
    story. As you are determined to continue upon your quest, he
    gathers up some things and accompanies you back to the
    roadside.

    Finally, as he turns to part, he says ‘These may be of some use
    to you. Farewell!’

    You can see:
    a shiny brass lamp
    some sandwiches
    a leather flask

    What now? GET ALL

    Your hands are full.

That message doesn't mean "You failed to get something"; it's just
observing that you _have now reached_ your limit, as far as non-pocketed
items go. That limit is 4. On the other hand, your pants pockets can hold
perhaps an unlimited number of items — certainly at least 11 — but
only those that are small: keys, map, coin, (dwarvish) axe and magazines,
pyramid, jewelry, diamond, emerald, and spices.

Anyway, now that we have the keys and lamp, we can finally enter the cave.

## The discs

As you might have guessed, _AST_ doesn't permit you to use the magic words
XYZZY or PLUGH to skip the introduction. But eventually we find the reason
for that: _AST_'s magic words have a material component!

    You are in a room filled with debris washed from the surface.
    A sediment of mud and rotting vegetation has filled the lower
    part of the cave making it possible to scramble up to an
    awkward canyon leading west.

    Scratched high on one wall are the mystic runes ‘XYZZY’.

    You can see:
    a black rod

    Your presence triggers some hidden mechanism and the ground
    seems to dissolve and reform before your eyes until...
    [more]
    You can see:
    a large silvery disc set into the ground

    What now? XYZZY

    A column of light blazes from the disc momentarily.

    What now? STAND ON DISC

    OK.

    What now? XYZZY

    In the blink of an eye...

    You are inside the well house. Cool, clear spring-water rises
    from a rocky trough in the floor and flows through an opening
    at the base of the south wall.

    A rectangle of light stretches from the doorway, illuminating
    some steps set against the far wall beneath a trapdoor to the
    ceiling.
    [more]
    You step off the disc.

(`STAND ON HEAD` serves just as well as `STAND ON DISC`;
_AST_'s two-word, four-letter parser doesn't care.)

There's a silvery disc set into the ground in the debris room,
and (now) an identical silvery disc set into the ground in the
well house. You can stand on either disc and say `XYZZY` to warp
to the other one.

And in the Y2 room, likewise... but this disc isn't set into the ground!

    You can see:
    a large silvery disc

    A hollow voice intones ‘Plugh’.

    What now? GET DISC

    OK.

This disc (which reminds me of the "portable hole" in
_[The Multi-Dimensional Thief](https://ifdb.org/viewgame?id=waov63ym41b481n9)_,
although that's totally different, really) works like one-half of a portal gun:
you can tote it around, drop it anywhere, stand on it, and warp straight to the
well house using the magic word `PLUGH`. Er...

    The voice says ‘I lied. The real word is...

        ‘GLUPH’

    A column of light blazes from the disc momentarily.

This helpfully balances out your reduced inventory, because you don't have
to tote treasures all the way back to Y2 — you can bring the magic to them.
There are only a few places where the disc isn't allowed: the west pit,
the troll bridge, and Witt's End. If you try to take it through one of those
rooms, it'll tingle in your hands and _poof!_ vanish, to be found in the
room you just left.

## Modified puzzles

Speaking of places you can't take the disc, you might expect that taking the
disc to the plover room (via `PLOVER`) would let you `GLUPH` the emerald out.
In fact you're correct — but only because _AST_ vitiates the plover
puzzle entirely! Using `PLOVER` doesn't vanish the emerald at all, in this game;
so you don't ever need to take the tight squeeze.

The troll complex is also a bit nerfed: specifically, the bear is gone —
nicked the troll's lunch and escaped! — so you merely need to throw
your _sandwiches_ to the troll. The bearless chain is easy to unlock, and when
you return to the bridge with it, the troll races off to find
the escaped bear. (Or pay in spices, if you didn't remember your keys; but
I don't think there are any food items in the game besides those two, so
a third crossing would be impossible.)

Most adventurers probably don't notice, but Don Woods' dragon is a technical
tour de force. You can reach the dragon's secret canyon from either direction;
behind the scenes there are really two different locations — "dragon seen from
the north" and "dragon seen from the east" — and a little bit
of code to teleport all the items from one to the other. When the dragon dies,
the items and player are teleported again to a third location —
"canyon without dragon." _A Serf's Tale_ eliminates the need for all this
machinery by moving the dragon around the bend, to the bottleneck between
slab room and mirror canyon, which can be approached only from the south.

_Adventure_'s "several diamonds" have become a "large diamond" and moved
one step westward; the Ming vase is now a "delicate porcelain figurine"
with the same fragile qualities.
The rare coins have disappeared entirely. Instead, there is a non-treasure
coin buried in the straw in the well house's attic; that's the coin you'll
need in order to recharge your lamp.

    It is a fully sealed dwarvish mining lamp. A stamp on the side
    reads ‘Made in Moria’.

## New puzzles

Like PLAT0550, _AST_ does something interesting with the reservoir.

    You are at the edge of a large underground reservoir. An opaque
    cloud of white mist hangs above the water. The lake is fed by a
    stream which gushes from a hole in the wall about ten feet
    overhead, splashing noisily somewhere in the mist.

    What now? SWIM

    Try a compass direction.

    What now? N

    You are standing waist-high in the chilly waters of a large
    underground reservoir.

    Something glimmers under the water.

    What now? N

    You swim out to the middle of the reservoir. There is a
    strange patch of light below you so, taking a deep breath, you
    dive down to investigate...

    You are swimming above a submerged passage that glows
    with an eerie light.

    What now? D

    An eerie phosphorous light fills the submerged passage, emanating
    from millions of tiny marine creatures that line the way as
    you swim through.
    [more]
    You are in a secret, half-submerged grotto. Stalactites
    hang across the room, meeting stalagmites in several places to
    form huge pillars. A stone door is set into the west wall.

    You can see:
    a golden conch

The door is a one-way passage to the oyster cul-de-sac — beautifully fitting!
I think this area is by far the most successful and enjoyable
of _AST_'s additions to the game.
(I was disappointed only that I couldn't `BLOW CONCH`.)

---

The other new puzzle is by far the _worst_ addition, in game-design
terms. The game is bookended by terrible unclued puzzles: at the beginning
the cutscenes, and at the end this "scarab complex."

Step 1: Go get all 15 of the treasures mentioned thus far,
and put them in the well house.

Step 2: Go to Y2, then `E` to WOOD0350's "jumble of rock" — but notice that
in this game it's a "jumble of soft earth and loose rock." So we can `DIG`,
then go `DOWN`:

    You are at the end of a long winding earthen tunnel that
    stretches away to the west. The roof is supported in places by
    wooden props.

A little further on:

    A smell of decaying timber fills the tunnel which looks as if it
    could collapse at any moment.

Press on from here and the tunnel will collapse:

    The walls begin to crumble and loose rocks shower down upon you.

    Great clumps of earth follow, piling around you. The timbers
    groan and buckle and finally the roof collapses, burying you
    alive!

Instead, you have to find some way to prop up the tunnel. Now,
the second time you crossed the troll bridge, a piece of timber fell off
into the chasm below. Suppose, _before_ crossing, you type `GET TIMBER`.
That item isn't listed in the room description, but it works!
Take that "six-foot timber prop" back to the sketchy tunnel and
`PLACE TIMBER` (or `PLACE PROP`, or `PROP ROOF` — but it's a bit guess-the-verb-y
no matter what), and then you can continue northwest:

    You are in an ancient vaulted chamber. Massive cobwebs are
    strung across the ceiling; dust lies upon every surface. In the
    centre of the room a three foot pedestal rises from a square
    stone plinth.

    You can see:
    a large silver scarab placed upon the pedestal

    What now? EXAMINE PEDESTAL

    You see nothing special.

    What now? GET SCARAB

    As you take it the pedestal creaks and starts to slide down
    into the ground!

You now have exactly one turn to stop the trap, or else the tunnel will,
again, collapse. One thing you can do to survive is just put it back:

    What now? PLACE SCARAB

    OK.

(What an anticlimactic message!) From this, an astute adventurer might
make a connection to _Raiders of the Lost Ark_ (1981)'s
[iconic opening scene](https://www.youtube.com/watch?v=mUWYmTpYdP4&t=458s)
and deduce that what we're supposed to do here is replace the scarab with
something else of approximately the same weight. Now, this pedestal is
(A) already moving, and (B) _sinking_, so it might seem both late and
counterproductive to add any weight to it at all. But let's try it anyway.

    What now? PLACE FLASK

    What?

    There is a loud click as the pedestal vanishes...
    The walls begin to crumble and loose rocks shower down upon you.
    [...]

This unhelpful message (followed by instant death) ought to indicate that
we're on the wrong track. But in fact this _is_ the intended solution —
we just need to randomly guess the item that weighs the same as a
"large silver scarab."

_Adventure_ metagamers will have already guessed the answer: since the golden
eggs weren't used for the troll puzzle, they must be used here!

    What now? PLACE EGGS

    OK.

(Again, so anticlimactic!) Now we leave the tunnel, _fee fie foe foo_ the eggs
back to the Giant Room (presumably collapsing the tunnel when they vanish),
and re-collect the eggs. Now we have 15 treasures in the well house, plus the
scarab.

Step 3: `READ SCARAB`. Engraved on its belly are the letters `* 5 MAGE DEN`.
This is the fifth part of a message sequence whose first four parts are found
by reading the black rod, the magazines, an impassable door at Volcano View,
and the plaque in the Dark Room. Put them together and they spell
`* N SW W SE MAGE DEN`.

Step 4: Recall that you saw a big `*` on the floor in the room where the pirate
keeps his treasure chest. Go back there and `N`, `SW`, `W`, `SE` (leaving you
somewhere in the maze); then say `MAGE DEN`.

Step 5: ...Did I mention _you need to be carrying the scarab?_ If you
deposited it safely in the well house with the other 15 treasures, nothing
happens. If you're still carrying it, though, then upon saying `MAGE DEN`,
you receive the final cutscene:

    From far away you hear a thin crackle, and a gong sounds.

    A sepulchral voice says ‘The Cave Closing mechanism has been
    activated. All employees to the Main Office... Will visitors
    please leave by the nearest exit. Thankyou.’

    As the last echoes fade the scarab's eyes glow red and there
    is a blinding flash of light!

    When your eyes refocus you see that a door has appeared in the
    far wall, so you step through, hoping to find a way out...

    Before you stands the Patron! ‘My friend!’ he says, ‘I am
    truly the Wizard Eru Tnevda. At last, after generations of
    waiting, someone has passed my test!’

    Some dwarves enter. They are smiling and slapping each other
    on the back. Then you see the troll. He twiddles his thumbs
    and says bashfully ‘No hard feelings, huh?’ Behind him the
    dragon nods in agreement.

    ‘Only you have the ingenuity’ the Wizard says, ‘to embark upon
    a yet greater quest!’ and smiling, he beckons you to follow.

    ...But that's another story!

You then receive a final count of rooms visited and treasures deposited,
and the game ends. (The treasure count seems off to me: I think it's
taking whatever count was displayed the last time you said `SCORE` and adding
1 to that. So if you never checked your score, it'll just say "1";
or it can go as high as "17".)

That's an absolutely lousy guess-the-verb instant-death series of puzzles,
followed by an awfully fiddly endgame. But after all, it was 1986, and it was
_Adventure_.

A complete walkthrough for the game is [here](/blog/code/2024-09-30-a-serfs-tale-walkthru.txt).

## A final challenge?

Recall that _A Serf's Tale_ was authored by Nigel Brooks and Said Hassan.
Enter either of their names (`NIGEL` or `SAID`), and the game responds with:

    That was clever of you! Now take the timber prop to the room with
    the pedestal.

The first several times I read this message, I thought it was supposed to
be a hint to the scarab complex. But if so, it's a _terrible_ hint,
because it's impossible even to discover the pedestal room
unless you've already solved the puzzle of propping the tunnel ceiling.
It's sort of an anti-hint.

But then I've begun to wonder... Was the message intended not as a hint but
as a challenge to the player? It _seems_ impossible to enter the pedestal room
while still carrying the prop; but maybe a clever enough player can do it.
(Back in 2019 [I wrote](/blog/2019/01/28/mcdo0551-sandbox-game/) about
a clever way to bring the bear and oyster back to the well house in MCDO0551.)

If you come up with a way to take the timber prop to the room with
the pedestal — please, let me know!

And remember, you can try out your solution
[here](https://spectrumcomputing.co.uk/playonline.php?eml=1&downid=42595).
