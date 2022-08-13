---
layout: post
title: "Observations on _Castlequest_'s code"
date: 2021-03-21 00:01:00 +0000
tags:
  adventure
  castlequest
  digital-antiquaria
---

Holtzman and Kershenblatt's _Castlequest_ was inspired by Crowther and
Woods' _Adventure_, and has a lot in common with it:

- cave-based treasure hunt with fantasy themes
- magic words of teleportation
- the "master game" and Last Lousy Point
- lamp, key(s), food, bottle
- wandering random-chance-of-death monsters; `THROW AXE`
- two liquids to fill the bottle with
- long and short room descriptions; `BRIEF`
- navigation by compass points, plus `U`/`D`/`IN`/`OUT`; `BACK`
- mazes of twisty passages

But since Holtzman had never seen _Adventure_'s code, some of the behind-the-scenes
mechanisms are pretty different. Let's take a look at a few key differences
between _Adventure_'s code and _Castlequest_'s.


## Parser structure

### Grammar in _Adventure_

_Adventure_'s grammar consists of four kinds of words:

- _Motion_ words, like `NORTH` and `SOUTH`
- _Object_ words, like `LAMP`
- _Action_ words, like `TAKE` and `DROP`
- _Message_ words, like `HELP` and `SWIM`

I said motion words were like `NORTH` and `SOUTH`, but actually those boring compass directions arrived very
late in _Adventure_'s gestation. _Adventure_'s first motion words were `ROAD`, `ENTER`, `UPSTREAM`,
`DOWNSTREAM`, `FOREST`. So you could stand outside the building and say `ENTER` to go inside, or `ROAD` to
follow the road instead.

Each object word identifies a specific object in the game. For example, the word `LAMP` identifies the lamp
(object number 2 in the game; number 1 is the keys).

Action words may be transitive (`FEED`) or intransitive (`QUIT`).

Message words, like `HELP` and `INFO`, are handled essentially outside of the game proper; when the game sees
one of these words in the command, it simply prints that word's associated hard-coded message and immediately
returns to prompting for input. Besides `HELP` and `INFO`, other message words in _Adventure_ include
`DIG` ("Digging without a shovel is quite impractical"), `SESAME` ("Good try, but that is an old worn-out
magic word"), `MIST`, and `TREES`.

Commands consist of one or two words, of any kinds. Motion words take precedence
over action or object words, so that when you say `JUMP FISSURE` the object word `FISSURE` is dropped, yielding `JUMP`;
and when you say `WALK WEST` the action word `WALK` is dropped, yielding `WEST`. (There are also a few special
cases, such as that when the first word is the motion word `ENTER`, we'll just look at the second word;
this makes `ENTER TUNNEL` a synonym for `TUNNEL`.)

Word order isn't important: _Adventure_ treats `AXE GET` and `GET AXE` as equally grammatical.
This is convenient when the user types just `AXE`: the game will ask "What do you want to do with the axe?"
and then if you answer `GET`, the game will concatenate the two commands (`AXE GET`) and ta-da, it's a valid command!
But if you answer with some other command, such as `NORTH`, the game will see `AXE NORTH` and the motion word
`NORTH` will take precedence.


### Grammar in _Castlequest_

_Castlequest_'s grammar consists of two kinds of words:

- Verbs
- Nouns

Verbs can be transitive (`FEED`) or intransitive (`UP`). Verbs 1 through 10 are the compass directions: `N`, `NE`,
`E`, `SE`, `S`, `SW`, `W`, `NW,` `U`, `D`. (Then `TAKE`, `DROP`; `IN`, `OUT`; `ATTACK`, `KILL`, `THROW`, `LOAD`,
and `SHOOT`.
Compare to _Adventure_'s first action words: `CARRY`, `DROP`, `SAY`, `UNLOCK`, `NOTHING`(!), `LOCK`, `LIGHT`,
`EXTINGUISH`, `WAVE`, `CALM`, `WALK`, and _then_ `ATTACK`.)

Commands consist of a verb, optionally followed by a noun.
The parser considers `AXE TAKE` to be ungrammatical nonsense. In fact, it considers `AXE` alone to be
ungrammatical nonsense, because the string of characters `AXE` is not recognized as a verb. (The parser
never tries to parse your first word as a noun.)

So when you see _Castlequest_ say "Do WHAT with the axe??",
it's because you said `FROTZ AXE` or something — it recognizes `AXE` because it's in noun position,
but doesn't recognize your first word as any known verb. And _Castlequest_ is not
equipped to handle a clipped reply like `TAKE` — it'll parse that as a brand-new command, and come
back with "I don't see that here" (where _that_ refers to the non-existent noun in your one-word command —
indeed object zero isn't here).

_Castlequest_ has no distinction between action and message words. This makes it more expensive for _Castlequest_
to add words like `SWIM` and `FUCK`, but also makes them as customizable as "action" verbs like `READ`
and `UNTIE`. For example, `SWIM` prints "I hate to tell you this, but I can't swim" in only four locations;
everywhere else gives a bland error message. And `FUCK` gives two different messages, depending
on whether you used it transitively or intransitively.

By the way, in both _Adventure_ and _Castlequest_, words can have multiple accepted spellings; for example,
the spellings `WEST` and `W` both mean motion word 44 in _Adventure_ and both mean verb 7 in _Castlequest_.
These spellings can look arbitrarily different to human eyes. For example, in _Castlequest_ both `BREAK`
and `CHOP` translate to verb 37; in _Adventure_ both `ROAD` and `HILL` translate to motion word 2.
_Adventure_ makes `ATTACK` and `KILL` synonymous; _Castlequest_ makes them two separate verbs.


## The travel table

### Travel in _Adventure_

_Adventure_'s vocabulary has more motion words than any other kind. Getting around the
game world is the main point of the parser engine, because it was (originally) the main point of Crowther's
game. The game world is a collection of rooms; you navigate between rooms using motion words. The fact that
you can also get lamps, water plants, and so on is almost an afterthought grafted onto this navigation engine.

So _Adventure_ has this thing called the "travel table." It's a massive data file encoded like this:

    1  2   2 44 29
    1  3   3 12 19 43
    1  4   5 13 14 46 30
    1  5   6 45 43
    1  8   63
    2  1   2 12 7 43 45 30
    2  5   6 45 46

This says that to get from room 1 (end of road) to room 2 (hill in road), you can enter motion
word 2 (`ROAD`/`HILL`), 44 (`W`), or 29 (`UP`).
To get from room 1 to room 3 (inside building), you can use 3 (`ENTER`), 12 (`HOUSE`), 19 (`IN`), or 43 (`E`).
Motion words 5, 13, 14, 46, 30 (`DOWNSTREAM`, `GULLY`, `STREAM`, `S`, `D`) take you to room 4 (valley).
Motion words 6, 45, 43 (`FOREST`, `N`, `E`) take you to room 5 (forest).
Finally, motion word 63 (`DEPRESSION`) takes you from room 1 to room 8 (outside grate).
The travel table continues with six ways to get from room 2 to room 1... and so on.
As a historical note, observe again that the compass directions were added late — they have pretty high numbers,
and they appear at the tails of these lines, which were probably accreted in chronological order. (Crowther decided
that the road led `WEST` from the building before Woods decided that it should also lead `UP`.)

Notice the richness of this format. It's easy to add a new motion word; by default it will do nothing in
every room, except those where you add it to the travel table. This allowed Crowther to add simple navigational
shortcuts, like `BUILDING` to move back to the starting room from pretty much anywhere above ground, and
of course also "magic words" like `XYZZY`. These shortcuts are useful in speed-running; why type `W W W W W`
to get from below the grate to the top of small pit, when you could get there in one turn with `PIT`?

The travel table can also encode more complex behaviors:

    19   35074   49
    19  211032   49
    19      74   66

The numbers over 1000 use an ad-hoc but powerful scheme to encode both probabilities and conditions
on certain motions. This snippet says: If you're in room 19 (hall of mt king) and you use motion
word 49 (`SW`), you have a 35% chance of reaching room 74 (secret E/W canyon). Otherwise,
if object 1011 (the snake) is also present here then you'll reach room 32.
Otherwise, you'll reach room 74 (secret E/W canyon), which by the way you can
invariably reach by using motion word 66 (`SECRET`). (Notice that travel table entries "fall through"
in the same way as C switch cases.)

Meanwhile, room 32 is a special room with only one exit in the travel table:

    32  19   1

Motion word 1 is a magic number that means "forced motion"; such exits are automatically
taken after describing the room you're in.

    You are in the hall of the mountain king, with passages off in all
    directions.
    A huge green fierce snake bars the way!
    > WEST
    You can't get by the snake.
    You're in hall of mt king.
    A huge green fierce snake bars the way!

To the player, it looks like the game has rejected your attempt to move; but what's actually
happened at the mechanical level is that you _successfully_ moved to a (lighted) room with the
description "You can't get by the snake," and then immediately moved back
to the Hall of the Mountain King following a forced motion.

Here's another interesting use of the travel table: The vocabulary word `BACK` is grammatically
a motion word, but it's actually intercepted in the parser. When you say `BACK`, the game
tries to move you back into the room you were in last turn, by substituting some other motion
word for `BACK`. It scans the travel table to find all the exits from your current room, and
sees if any of them lead into your previous room. To see this in action, go `S` twice from
end of road to reach slit in streambed; then say `HOUSE` to move back to end of road;
then say `BACK`. "You can't get there from here," because the direct connection between these
two rooms is one-way. Of course this mechanic can also be used to create "in-universe"
one-way connections as well, such as the steep incline north of the Giant Room.


### Movement in _Castlequest_

_Castlequest_'s movement system is much simpler. The first ten verbs are the compass directions;
these are the primary way of navigating in _Castlequest_. All ten of those verbs are handled by
the `MOVE` subroutine, which consults a hard-coded 10×100 array. (There are exactly 100 rooms
in _Castlequest_.)

    C          N  NE   E   SE  S  SW   W  NW  UP  DOWN
      DATA W1 /0,  0,  0,  0,  0,  0,  2,  0,  0,-29,
     2         3,  0,  1,  0,  0,  0, -4,  0,  0,  3,
     3        33,  0,  0,  0,  8,  0,  5,  0, -2,  0,

This says that from room 1 (the bedroom), you can exit to the west or down — no other directions!
The exit for "down" is strange; the negative number indicates that some
special handling is needed. So when you try to go `DOWN` from the bedroom, the game sets
`LROOM` to 1 and `ROOM` to 29, just as usual, but then checks a ton of special cases.
One of those cases is:

         IF ((ROOM .NE. 29 .OR. LROOM .NE.  1) .AND.
        2    (ROOM .NE.  1 .OR. LROOM .NE. 29)) GOTO 621
         IF (ROPE .EQ. 2) GOTO 103
         WRITE(6,1006)
         GOTO 106
    106  ROOM = LROOM
         LROOM = II
         GOTO 25
    1006 FORMAT('0  There is no way to go in that direction.')

So if you try to go `DOWN` when global variable `ROPE` isn't set to `2` (which in-game means
the rope is dangling out the window), you'll get a bland error message and your `ROOM`
will get reset to 1.

The main 10×100 array is supplemented by two more 100-element arrays, `ENTER` and `LEAVE`,
which indicate whether the verbs `IN` and `OUT` work in the current room. These lookup
tables don't map to destinations, but rather to _verb numbers_ — comparable, but not equivalent,
to _Adventure_'s handling of the word `BACK`. When you're in the bedroom and you say `OUT`,
that gets remapped to verb 7 (`WEST`). When you're in the library, `IN` gets remapped to verb 10
(`DOWN`), which then triggers its own special-case handling.

An odd effect of this mechanism is that the "level designer" can remap `IN` and `OUT` to verbs
that aren't directions at all. Holtzman did this exactly once — I guess just for the heck of it.
Going `IN` from room 61 maps to verb 19 (`FUCK`)!

    You are in the blue room.  The entire room is a deep
    shade of royal blue.  Exits go north, south and east.
    > N
    You're in a maze of short and winding passages.
    > NE
    You're in a maze of winding, long passages.
    > IN
    You had better watch your mouth.

If the `ENTER` mapping for the current room doesn't exist, typing `IN` gives a bland error message.
But if the `LEAVE` mapping for the current room doesn't exist, `OUT` is remapped to `BACK`!
Thus, if you go `OUT` to leave a room, and then type `OUT` again, you'll generally find yourself
right back `IN`; this confused the heck out of me, before I looked at the code.

Finally, when you type `BACK` (verb 40), the game tries to move you back into the room
you were in last turn. Unlike in _Adventure_, this does not consult any travel table: `BACK`
always succeeds, unless some unusual event has changed the value in variable `LROOM`.
For example, when you exit the mirror maze into a random room, you can always say `BACK` to
return to the maze, even when the maze isn't ordinarily adjacent to your random room.

Verbs such as `POOF` and `CROSS` are handled in the same way as any other non-motion verb:
with special-purpose codepaths. And `CLIMB` is merely a synonym for `UP`.


## The wandering monsters

### _Adventure_'s dwarves and pirate

_Adventure_ has five dwarves that wander around the map and throw knives at you. Surprisingly,
a large subset of _Adventure_'s codebase is dedicated to simulating these little guys. Each
dwarf's state is represented by a set of entries in three arrays (`ODLOC`, `DLOC`, `DSEEN`).
Just like the player, each dwarf has a current location `DLOC` and a previous location `ODLOC`.
Dwarves avoid doubling back on their own paths, but otherwise wander randomly through the cave,
respecting the travel table. Each time the player moves, the dwarves also move. Whenever a
dwarf winds up in the same room as the player, we set `DSEEN` and the dwarf starts preferentially
following the player and throwing knives. Since there are five dwarves wandering randomly,
it's quite possible for the player to see messages like

    There are 4 threatening little dwarves in the room with you!
    3 of them throw knives at you!
    2 of them get you!

The travel table can mark certain paths, such as the troll bridge, as "forbidden to dwarves."

There is a sixth dwarf — the pirate — who gets special handling once he enters your room,
but otherwise wanders according to the travel table just like any other dwarf.
When the pirate moves into your room but you have no treasure to steal,
there's a 20% chance that "There are faint rustling noises from the darkness behind you."

When you spot the first dwarf (the one who throws the axe and runs away), the game randomly
kills between 0 and 2 of the dwarves, leaving only 3 to 5 of them alive (plus the
pirate). Once you kill a dwarf, it never comes back; so you should need to kill only at
most 5 dwarves in any single game of _Adventure_.

By the way, Luckett and Pike's _Adventure II_ ([taxonomized](http://advent.jenandcal.familyds.org/#LUPI0440) as LUPI0440)
grants three of the dwarves the ability to pick up and carry a single item as they wander,
and also sometimes randomly revives dead dwarves.

The dwarf-movement parts of _Adventure_ are one of the coolest parts of the implementation —
that's probably why Crowther bothered to write all that code — but they're also the part that
tends not to survive porting to other game engines. [PLAT0550](http://advent.jenandcal.familyds.org/#PLAT0550)
keeps track of between 4 and 8 dwarves, but they don't "wander" so much as "teleport to the player"
at random times, until they're all dead.
[MALM0350](http://advent.jenandcal.familyds.org/#MALM0350) doesn't even bother
to track a population size; there's just a flag to tell whether the (one) dwarf is in your room,
and when you kill the dwarf, it merely turns off that flag for a while.


### _Castlequest_'s werewolf and gnome

_Castlequest_ uses the MALM0350 model for both of its wandering monsters. Every turn you're
in the castle, there's about a 4% chance that the werewolf will show up. When it does, it sets
the `WOLF` flag, meaning that it'll stick to you forever, or until you kill it. There's no way
to shake it from your trail — not boating, not `POOF`-ing, not climbing ropes.

Likewise, every turn you're in the cave, there's about a 3% chance that the gnome will show up
(setting the `GNOME` flag). The gnome's description is an homage to _Adventure_ —

    There is an ugly little gnome in the room with you!
    He shoots a poisoned dart at you!
    IT GETS YOU!!

— which is also a hint to the seasoned adventurer that the proper way to deal with gnomes is
to `THROW AXE`. This gives you a reason to tote the axe while in the cave, and thus to have
a good chance of discovering the secret of the master game. Neither the werewolf nor the
gnome ever perma-die.

_Castlequest_ will sometimes print "I think I hear footsteps behind you" — with probability 0.8%
every time you move — unrelated to anything else going on in the game. (Remember,
the werewolf and gnome don't "move" the way _Adventure_'s pirate does; they just teleport in
randomly.)


## Doors and windows

### _Adventure_'s `PROP` array

In _Adventure_, the `PROP` array holds one integer for each object in the game, both movable and immovable.
This integer represents the "state" of the object. The default state is 0; some objects (such as the iron keys)
never leave that state. The lamp has two possible states: `PROP(LAMP)=0` when it's off and `PROP(LAMP)=1` when
it's on. The bear has four states: hungry, tame but chained, unchained, and dead. Each one has a different
description.

These states are generally _written_ via special handling in the code, but the travel table can encode
_reads_ against them. For example,

    8  303009   3 19 30
    8     593   3

indicates that typing any of `ENTER`, `IN`, `DOWN` in room 8 (depression) will lead to room 9 (below grate)
only if `PROP(3)=1` — that is, if object 3 (the grate) is in state 1 (open).
Likewise, the path north of the Giant Room is open only if `PROP(DOOR)=1`.


### _Castlequest_'s `DOOR` array

In _Castlequest_, items don't have "props"; there is no general-purpose way for an item to have state,
and no general-purpose way to change the description of an item. Each stateful puzzle in the game is
represented in the code by a named global variable; for example, the integer `BAT` may hold 0 (gone)
or 1 (present); the integer `ROPE` may hold 0 (loose), 1 (tied to bed), 2 (out window), or 3 (tied to hook).
(`ROPE` can also be -2 if it's fallen out the window, but I'm not sure why that needed to be different
from 0.)

The general-purpose array in _Castlequest_ is `DOOR(100)` — one integer per room. The game uses this
array to track the state of at most one door per room. `-2` means "the door will
neither open nor close"; `-1` means "I see no door here"; `0` means locked, `1` means closed, and `2` means
open.

For example, `DOOR(2)`, `DOOR(6)`, `DOOR(21)`, and `DOOR(80)` are all initially zero,
because these rooms (the dim corridor, the kitchen, the attic, and the wine cellar) all
contain locked doors. The code for locking, unlocking, opening, and closing these doors is
mostly generic. Movement _through_ these doors, on the other hand, must be handled via special
cases (negative numbers) in the 10×100 travel array.

The library contains an initially open door, which can be locked, unlocked, opened, and closed
generically.
The same is true of the locked room (next to the dim corridor) and the brick wall (next to the kitchen).
Of course, the game doesn't "know" that these rooms' `DOOR` entries are supposed to represent
the two sides of the same actual door, so you might find that the door appears locked from one direction
and open from the other.
But since these rooms also aren't hooked up to any special cases for movement, such a locked door
won't impede your travel.

    You are in the upstairs hallway, a long corridor with passages
    to the north, east, and west.  Stairs lead up and down.
    > OPEN DOOR
    The door will neither open nor close.
    > EAST
    You are in the library.
    A copy of Shakespeare's "HAMLET" lies on the desk.
    > OPEN DOOR
    The door is already open.
    > CLOSE DOOR
    OK
    > LOCK DOOR
    The door is locked.
    > WEST
    You're in the upstairs hallway.
    > EAST
    You are in the library.
    > OPEN DOOR
    The door is locked.

The windows in the bedroom and smoking room are handled by separate global variables `WIND1` and `WIND2`,
respectively, and don't interact with the `DOOR` array at all. You might wonder why _Castlequest_
bothers to make a whole 100-element array just to deal with four special doors. I don't know. But
the game _does_ start off inside a house, where you might expect every room to have a door; maybe
it initially seemed like simulating doors was going to be a big part of the game.


## The `INVENTORY` command

In _Adventure_, the `PROP` of an object tends to change its long description (when present in the current room),
but never changes its name-when-inventoried. Rather than having separate items for "empty bottle"
and "bottle of water," _Adventure_ simply has an item with name `Small bottle` and another slightly special
item with name `Water in the bottle`; when you say `FILL BOTTLE` it places the latter in your
inventory right next to the former. For the bird, it cheats even more: the short name of the bird
is `Little bird in cage`, because whenever it's in your inventory it is in the cage by definition.

In _Castlequest_, objects tend _not_ to change their long descriptions; the lamp keeps its same description
no matter whether it's on or off. But items in your inventory can combine in interesting ad-hoc ways; for example,
if you are carrying both the gun and the bullet, and the global `GUN` flag is true (meaning that
you've loaded the gun), then `INVENTORY` will print

    Bullet in gun

instead of

    Silver bullet
    Old gun

The way it accomplishes this is... local hacks in the `INVENT` subroutine.
The gun and bullet are items 20 and 2 respectively:

    IF (.NOT. GUN) GOTO 120
        ITEMS(20) = 0
        ITEMS( 2) = 0
        WRITE(6,2000) 'Bullet in gun'

The short description of the bottle in _Castlequest_ is `Empty bottle`; so, when you are also carrying
object 5 (`Blood in bottle`), the inventory code will temporarily suppress the printing of object 18:

       IF (BOTTLE) ITEMS(18) = 0
    [...]
       DO 10 II=1,NITEMS
         IF (ITEMS(II) .EQ. -1) WRITE(6,2000) OBJ(II)
         IF (ITEMS(II) .EQ. -1) NUMB = NUMB + 1
    10 CONTINUE
       IF (BOTTLE) ITEMS(18) = -1

Incidentally, both _Adventure_ and _Castlequest_ make the same arbitrary decision to track
"number of items carried by the player" separately from "location of each item." In theory,
the number of items carried by the player should always be exactly the same as the count of items
in room `-1`. In practice, though, tracking these quantities individually means there's no
single source of truth, and the two quantities can get out of sync with each other.
See ["A bug in _Adventure_'s endgame"](/blog/2020/02/06/water-bottle-bug/) (2020-02-06).
I haven't explicitly found such a bug in _Castlequest_ yet, but I'm sure it's only a matter
of time.

_Castlequest_'s combining-item hacks are pretty much only in the inventory code (except
some paths related to the rope and grappling hook in the room-description code). If you're
carrying the loaded gun and you drop it, it magically unloads itself and you see the bullet
and gun separately in the room description. The rope and grappling hook also tend to spontaneously
disassemble; even the bottle of blood!

    > INVENTORY
    You are carrying the following object:
       Blood in bottle
    > DROP BOTTLE
    OK
    > LOOK
    You are at the bottom of a towering spiral stairway.
    A low passage exits south.
    There is a small pool of blood here.
    An empty bottle is discarded nearby.


## Conclusion

Oh, you expected a conclusion?

Well, it's pretty neat how _Castlequest_ invented its own solutions to certain problems without
seeing _Adventure_'s. In some cases the games demonstrate convergent evolution (for example,
how items in room number zero are "destroyed" and items in room number `-1` are "in inventory");
in other cases they're creatively divergent (such as _Castlequest_'s handling of `IN` and `OUT`,
or _Adventure_'s simulationist handling of `BACK`).

Anyway, this probably concludes my series of _Castlequest_ posts, at least for a little while.
Revisit the first post here:

* ["_Castlequest_ exhumed!"](/blog/2021/03/09/castlequest/) (2021-03-09)
