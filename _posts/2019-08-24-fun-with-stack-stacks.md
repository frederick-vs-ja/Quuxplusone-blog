---
layout: post
title: "Church booleans in Befunge-98"
date: 2019-08-24 00:01:00 +0000
tags:
  esolang
excerpt: |
  I like [this Code Golf question on "Church booleans."](https://codegolf.stackexchange.com/questions/190325/church-booleans)
  A "Church boolean" is either of the following two functions (in C++17 syntax):
---

{% raw %}
I like [this Code Golf question on "Church booleans."](https://codegolf.stackexchange.com/questions/190325/church-booleans)
A "Church boolean" is either of the following two functions (in C++17 syntax):

    auto T = [](auto t, auto f){ return t; };
    auto F = [](auto t, auto f){ return f; };

So you can build up Boolean operations such as

    auto NOT = [](auto x){ return x(F, T); };

    static_assert(NOT(T)(0,1) == F(0,1));
    static_assert(NOT(F)(0,1) == T(0,1));

and

    auto AND = [](auto x, auto y){ return x(y, F); };

    static_assert(AND(T,T)(0,1) == T(0,1));
    static_assert(AND(T,F)(0,1) == F(0,1));
    static_assert(AND(F,T)(0,1) == F(0,1));
    static_assert(AND(F,F)(0,1) == F(0,1));

See my C++17 solutions [here](https://codegolf.stackexchange.com/a/190384/11791)
and [here (curried)](https://codegolf.stackexchange.com/a/190444/11791).

Incidentally, I was surprised to find that a non-`constexpr` variable
of lambda type can still be called in constexpr context. I'm not sure
exactly what's going on there.

----

I wondered if the Code Golf puzzle could be solved in my favorite esoteric
programming language, Befunge.

I was already confident that there would be no sane way to define a
"higher-level function" in Befunge-93. You can reuse code by directing
the program's control flow back over itself, but control flow
in Befunge-93 is still entirely static; there's no way to "name" a
function so that it can be "called" from anywhere.

But [Befunge-98](https://github.com/catseye/Funge-98/blob/master/doc/funge98.markdown#funge-98-final-specification)
is overcomplicated enough that there ought to be a way to do it!

First I looked at the "fingerprint" feature, which lets you use
special subroutine-like semantics for instructions `A` through `Z`.
But there's no way to _define_ those semantics within Befunge-98.
(There is the [Dynafing](http://www.club.cc.cmu.edu/~ajo/funge/dynafing-mirror.html) spec,
which is close! But by the time I remembered that Dynafing existed, I was on to my next
idea.)

----

I looked for some way to do an "absolute jump" in Befunge-98.
If I could do absolute jumps, then I could "name" a function by
its two-dimensional address. Just as in real life, "calling" a function
would mean pushing a return address and jumping, and "returning"
would be implemented as jumping back. Unfortunately, Befunge-98
doesn't have any "absolute load IP" instruction.

But it does have an "absolute load delta" instruction! The `x` instruction
loads (dx,dy) from the stack; so for example `01x` means "go south from the `x`"
and `20x` means "go east from the `x` at double speed." So if I could find a
way to store the _current IP_ to the stack, then I could do some math and then
`x` to accomplish an absolute jump. (Of course anywhere I jumped had better
have a `>` "landing pad," to curtail my IP's insane velocity.)

How do I get a copy of the current IP in Befunge-98?
It doesn't seem to be easy. But I found that when you use the `{` "Begin Block"
instruction, in addition to creating a new stack on the _stack stack_,
it also resets the _storage offset_. It sets the storage offset to the value of
the current IP! And then, even better, it pushes the _old_ storage offset onto
the second-on-stack-stack.

So `{` sets the storage offset to the current IP;
then a second `{` pushes that storage offset onto the SOSS;
and then I can use `u` "Stack under Stack" to transfer those coordinates
up into my stack. Finally, a couple of `2}`s put me back in my original
execution context, with a copy of the storage offset.

    0{{2u2}2}
        Push your IP's current address.
        0{{2u2}2}  is the same as  y x
        where (x-1, y) is the address of the first `{`

----

Two-dimensional addresses take up two stack entries each, rendering Befunge's
bare-bones stack-reordering command `\` pretty helpless. But with `{` on the brain,
we can write up some helpers:

    abcdef n > :2+{\1u\1u\03-u}
        Bury the top-of-stack entry under n other entries.
        12345 1 :2+{\1u\1u\03-u}  is the same as  12354
        12345 3 :2+{\1u\1u\03-u}  is the same as  15234

    abcdef n > 1+:{3u\01-u\01-u\}
        Dig up a stack entry from under n other entries.
        12345 1 1+:{3u\01-u\01-u\}  is the same as  12354
        12345 3 1+:{3u\01-u\01-u\}  is the same as  13452

And then we can put together our "find my absolute address"
snippet to produce a "jump to this absolute address" snippet.

    n >0{{2u2}2}$-063*-\x
        Jump to (0, n), where we hope to find a `>` landing pad.

That snippet assumes that the `>0{`... begins in
column 0 so that the `x` instruction itself appears in column
18. If it begins in some other column, you'll have to
adjust the `63*-` accordingly. And if you want to actually _detect_
which column it begins in, by saving the x-coordinate returned from
`0{{2u2}2}`, then the code gets much more complicated; it'll have to
use the dig/bury helpers above.

----

Incidentally, this helper is also useful:

    > N1+u:N2+{1u\1u\03-uN1+}0N1+-u
        Copy the entry from under N other entries on the SOSS, to the top of the TOSS.
        (Remember that the SOSS is 2 entries bigger than you might think, because of the saved storage offset.)
        12345 0{ 678 31+u:32+{1u\1u\03-u31+}031+-u  is the same as  12345 0{ 6784
        67890 0{ 123 41+u:42+{1u\1u\03-u41+}041+-u  is the same as  67890 0{ 1238

----

So, we can make our Church booleans! Our input expression will be
a stack of function names (that is, line numbers; that is, y-coordinates),
which we feed to our fundamental building block `EXEC`, defined as

    > 0{{2u2}2}$-0a9+-\x

`EXEC` will pop a function name from the stack and jump there. Each of our
Church-boolean conjunctions will pop its operands and then `EXEC` the computed
result. So `TRUE` and `FALSE` might look like

    > $ 0{{2u2}2}$-0ab+-\x  FALSE
    > \$ 0{{2u2}2}$-0ac+-\x  TRUE

`OR` might look like

    > 3\ 0{{2u2}2}$-0ac+-\x    OR (assuming TRUE is on line 3)

For example, if the stack (from top to bottom) holds `OR TRUE FALSE 17 42`, we'll kick
things off by jumping to `OR`, which conceptually pops `TRUE FALSE` and execs `TRUE`
(but in fact tucks `TRUE` and then jumps to `TRUE`, which itself pops `TRUE FALSE`
and execs `TRUE`). Anyway, `TRUE` then pops `17 42` and jumps to `17`.
Line 17 might be a function that prints "the result is true" and halts.

This gives us something kind of like [Polish notation](https://en.wikipedia.org/wiki/Polish_notation)...
but not really, because of how it never distinguishes "operators" from "values."
For example, if the stack holds `OR TRUE NOT TRUE 17 42`, we'll exec `OR`, which pops
`TRUE NOT` — and then since `NOT` isn't a Church boolean, the whole thing derails.
We end up exec'ing `TRUE`, which pops `TRUE 17` and execs `TRUE`, which pops `42 0`
and execs `42`. I'm not sure what to call this kind of execution model, nor whether
it's good for anything.

Of course in order to be able to "name" `TRUE`, the programmer-of-`OR` has to know what line number
`TRUE` is on. And if the programmer-of-`TRUE` knows what line number `TRUE` is on,
then `TRUE` doesn't need the `0{{2u2}2}` dance to figure out its own y-coordinate.

----

Conclusion: I made ["Church booleans" in Befunge-98](https://codegolf.stackexchange.com/a/190810/11791),
and found a way to jump to any absolute address from within a Befunge-98 program
(even if it ended up not really being needed in this case), and ways to dig and bury
entries on the Befunge-98 stack.
{% endraw %}
