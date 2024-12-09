---
layout: post
title: "The best engineering interview question I've ever gotten, Part 2"
date: 2022-01-07 00:01:00 +0000
tags:
  sre
  war-stories
---

Before you read this post — which contains spoilers — you should read
["The best engineering interview question I've ever gotten, Part 1"](/blog/2022/01/06/memcached-interview/)
(2022-01-06).


## The challenge: Modifying memcached

Via its `incr` and `decr` commands, memcached provides a built-in way to
atomically add $$k$$ to a number. But it doesn't provide other arithmetic
operations; in particular, there is no "atomic multiply by $$k$$" operation.

Your programming challenge: <b>Add a `mult` command to memcached.</b>


## Analysis

This is a great engineering interview problem because it pretty cleanly
partitions the candidate pool into three different types:

Type 0 is just completely stumped by the challenge of interacting with a
real codebase. I don't think many people in this category would get all
the way to this point in the interview process anyway. But if you discover
that the candidate is in this group, well, don't hire them.

> By the way, MemSQL-in-2013 was doing deeply arcane and high-performance C++11,
> so the fact that this challenge incidentally requires fluency in C was a plus,
> not a minus, for their purposes. If your codebase is all Python and Go,
> you probably wouldn't use memcached for your interview challenge.

Type 1 looks at the problem and says, "Ah! I know just how to do this!
Multiplication is just repeated addition, and we already have a
ready-made addition subroutine in the form of `incr`. So I'll just
build on top of that. Ah, but instead of adding a constant to `x`'s value,
we need to add `x`'s value to itself... and the whole thing needs to be atomic.
Let's look at how the locking works..." They spend all three hours
[UPDATE: <a href="https://news.ycombinator.com/item?id=31065845">one hour</a>] getting
deeper and deeper down various rabbit holes, and never produce anything that works.
Candidates in this group don't get hired.

Type 2 looks at the problem and says, "Ah! I know just how to do this!
Multiplication is just like addition, except wherever addition does `+`,
I should do `*`." So they copy-and-paste, change all the `+`s to
`*`s, and they're done in 90 minutes. Candidates in this group stand a
really good chance of being hired.

The best candidates will notice they've
got lots of time left and polish their submission, for example by making
sure the formatting is consistent, adding unit tests, or revisiting
their "design decisions" to make sure they can justify them if asked.


## Walkthrough

To make sure my time estimates were in the right ballpark,
yesterday afternoon I did the whole thing myself.

> This section is likely to bore all but the most masochistic readers,
> so feel free to skip down to [the Conclusion](#conclusion).

I started by grepping for `"incr"` (with the quotes), since we want
to imitate the existing `incr` command, and it must be parsed _somewhere_.
That led me to a part of the `process_command` function that looks like this:

    } else if ((ntokens == 4 || ntokens == 5) && (strcmp(tokens[COMMAND_TOKEN].value, "incr") == 0)) {
        process_arithmetic_command(c, tokens, ntokens, 1);
    } else if ((ntokens == 4 || ntokens == 5) && (strcmp(tokens[COMMAND_TOKEN].value, "decr") == 0)) {
        process_arithmetic_command(c, tokens, ntokens, 0);

The argument with value `0` or `1` corresponds to the parameter `bool incr`.
I changed that to `int opcode` and changed these callers to

    } else if ((ntokens == 4 || ntokens == 5) && (strcmp(tokens[COMMAND_TOKEN].value, "incr") == 0)) {
        process_arithmetic_command(c, tokens, ntokens, 1);
    } else if ((ntokens == 4 || ntokens == 5) && (strcmp(tokens[COMMAND_TOKEN].value, "decr") == 0)) {
        process_arithmetic_command(c, tokens, ntokens, 0);
    } else if ((ntokens == 4 || ntokens == 5) && (strcmp(tokens[COMMAND_TOKEN].value, "mult") == 0)) {
        process_arithmetic_command(c, tokens, ntokens, 2);

(These magic numbers were a pretty bad design decision, but the quick decision
keeps me moving forward. Ten minutes later, I'll realize a better solution and
revisit this code.)

I skim over the body of `process_arithmetic_command` looking for references to
incrementing and decrementing. The error message
`"CLIENT_ERROR cannot increment or decrement non-numeric value"`
seems a little suboptimal, so I change that code to

    if (opcode == 2) {
        out_string(c, "CLIENT_ERROR cannot multiply non-numeric value");
    } else {
        out_string(c, "CLIENT_ERROR cannot increment or decrement non-numeric value");
    }

And similarly just below:

    +if (opcode == 2) {
    +    c->thread->stats.mult_misses++;
    +} else if (opcode == 1) {
    -if (incr) {
         c->thread->stats.incr_misses++;
     } else {
         c->thread->stats.decr_misses++;
     }

Mental note that I'll have to add a `mult_misses` field to whatever `c->thread->stats`
is; but for now, press onward. If I forget, the compiler error will remind me.

    -switch(add_delta(c, key, nkey, incr, delta, temp, NULL)) {
    +switch(add_delta(c, key, nkey, opcode, delta, temp, NULL)) {

Grep downward for `add_delta`.

     enum delta_result_type do_add_delta(conn *c, const char *key, const size_t nkey,
    -                                    const bool incr, const int64_t delta,
    +                                    const int opcode, const int64_t delta,
                                         char *buf, uint64_t *cas,
                                         const uint32_t hv) {

This signature violates my [guidelines for const-correct code](/blog/2019/01/03/const-is-a-contract/#guidelines-for-reliably-const-correct-code)
in that it passes a lot of things "by const value," but let's not take the bait.
Replace `bool` with `int` and keep going.

Finally, we've found the place we were looking for — the place where we need to change `+` to `*`!
This codepath becomes:

    +if (opcode == 2) {
    +    value *= delta;
    +    MEMCACHED_COMMAND_MULT(c->sfd, ITEM_key(it), it->nkey, value);
    +} else if (opcode == 1) {
    -if (incr) {
         value += delta;
         MEMCACHED_COMMAND_INCR(c->sfd, ITEM_key(it), it->nkey, value);
     } else {
         if(delta > value) {
             value = 0;
         } else {
             value -= delta;
         }
         MEMCACHED_COMMAND_DECR(c->sfd, ITEM_key(it), it->nkey, value);
     }

Mental note to implement `MEMCACHED_COMMAND_MULT`, and press onward.
A little further down, note that `slab_stats` needs a `mult_hits` field.

We've reached the end of `do_add_delta`. Wait, this is `do_add_delta`... so
what's `add_delta`? Ah, it's called from two places. And the first place sets
`bool incr` to `c->cmd == PROTOCOL_BINARY_CMD_INCREMENT`. Grepping for
`PROTOCOL_BINARY_CMD_INCREMENT` reveals that there's an enumeration of all
the commands in `protocol_binary.h`! I should use that. Add
`PROTOCOL_BINARY_CMD_MULTIPLY` to that enumeration, and refactor all of the
work I've done so far to use `PROTOCOL_BINARY_CMD_{DECREMENT,INCREMENT,MULTIPLY}`
instead of the magic numbers `0,1,2`. `int opcode` can stay as an `int`, since
grepping for the enumeration type's name (`protocol_binary_command`) reveals
that literally nothing in the codebase uses that type by name.

Implementing `MEMCACHED_COMMAND_MULT` in `trace.h` tells me that I also need a macro
named `MEMCACHED_COMMAND_MULT_ENABLED`. Where's that used? It's not. Okay. Add it anyway.
([Chesterton's Fence](https://www.chesterton.org/taking-a-fence-down/): If I don't
know why these `_ENABLED` macros exist, then I certainly shouldn't try to do anything
novel with my new one. I'll follow the herd.)

Finishing up the remaining compiler errors, I add a `mult_hits` field to `struct slab_stats`,
right next to `incr_hits` and `decr_hits`.
`git grep incr_hits` shows lots of places it's used; when I'm done,
`git grep mult_hits` shows the same number of uses. The line

    out->incr_hits += stats->slab_stats[sid].incr_hits;

is sneaky because I need to modify my copy of it in _two_ places.
I also add a `mult_misses` field to `struct thread_stats`, and change

    if (c->cmd == PROTOCOL_BINARY_CMD_INCREMENT) {
        c->thread->stats.incr_misses++;
    } else {
        c->thread->stats.decr_misses++;
    }

into

    switch (c->cmd) {
        case PROTOCOL_BINARY_CMD_INCREMENT: c->thread->stats.incr_misses++; break;
        case PROTOCOL_BINARY_CMD_DECREMENT: c->thread->stats.decr_misses++; break;
        case PROTOCOL_BINARY_CMD_MULTIPLY: c->thread->stats.mult_misses++; break;
    }

We don't technically need to change `add_delta` itself from taking a
`const int incr` to taking a `const int opcode`, but I think it's a good idea,
so I do it.

I reach the "code complete" milestone in 25 minutes. Let's try it out!

    set age 0 3600 2
    37
    STORED
    mult age 10
    27

Aw, crap.

I return to the place where the multiplication is supposed to be happening...

    if (opcode == 2) {
        value *= delta;

Ha! That should be using my new `PROTOCOL_BINARY_CMD_MULTIPLY`. I fix that.
In fact, I grep for `opcode ==` and fix a few more places I'd missed.
I reach the "code _really_ complete" milestone in 32 minutes. This time,
the code really does seem to work. I run a few manual tests:

    set age 0 3600 2
    37
    STORED
    mult age 10
    370
    mult age 2
    740
    mult age -1
    CLIENT_ERROR invalid numeric delta argument

    set fullname 0 3600 10
    John Smith
    STORED
    mult fullname 1
    CLIENT_ERROR cannot multiply non-numeric value
    mult
    ERROR
    mult bogus 1
    NOT_FOUND

I check its behavior on integer wraparound, and I check the syntax
`mult age 10 noreply` to make sure that's also supported. Since I
implemented everything by copy-and-paste, there's basically no way
these things _won't_ work just as well as they work for `incr` and
`decr`.

Hmm... with all this manual testing, I should probably write some actual
tests. Are there tests in the repo? Yes, under `t/`. `make test` builds
and runs them. So, I copy `t/incrdecr.t` into `t/mult.t` and modify it.
I reach the "code _and tests_ complete" milestone in 50 minutes.

> I imagine a candidate who didn't mess with the tests would still pass
> the interview; priorities in an interview are different from priorities
> when making a pull request. Therefore this is a great place for even
> the most introverted candidate to raise their head and interact a little
> bit: "I think I've got something that works; do you want to take a look?"

I see there's more tests in `binary.t`. I guess I should take a look at them too,
even though I don't feel like it. Yeah, yikes, there's another copy of the command
enumeration in there; I should add `CMD_MULTIPLY` to it, at least.

I should also add tests for the new stats, in `stats.t`. (Actually, because
one of these tests simply counts the number of stats returned, and I've added
two more stats, that test would _fail_ if I didn't modify it.)

Around the 60-minute mark I hit the "code _and tests_ complete" milestone
for the second time.

But as I puzzle my way through `t/udp.t`, I find a lot of tests devoted
to the "binary protocol" (as opposed to the plain-text protocol we talked about
in the problem statement). Should I modify the binary protocol as well as the
plain-text one? Actually, I already have, thanks to this mindless diff in the
function `dispatch_bin_command`.

        case PROTOCOL_BINARY_CMD_INCREMENT:
        case PROTOCOL_BINARY_CMD_DECREMENT:
    +   case PROTOCOL_BINARY_CMD_MULTIPLY:
            if (keylen > 0 && extlen == 20 && bodylen == (keylen + extlen)) {
                bin_read_key(c, bin_reading_incr_header, 20);
            } else {
                protocol_error = 1;
            }
            break;

But higher up, I see "quiet" versions of the same opcodes:

    case PROTOCOL_BINARY_CMD_INCREMENTQ:
        c->cmd = PROTOCOL_BINARY_CMD_INCREMENT;
        break;
    case PROTOCOL_BINARY_CMD_DECREMENTQ:
        c->cmd = PROTOCOL_BINARY_CMD_DECREMENT;
        break;

I'm not sure what those are for, but in order to do my copy-paste trick in
`t/udp.t`, I'll have to add one of these for `mult`. (Chesterton's Fence again:
I don't know why these "quiet" opcodes are important, but if `incr` and `decr`
have them, then `mult` should have one too.) So I add `PROTOCOL_BINARY_CMD_MULTIPLYQ`
and propagate that change through the codebase.

At this point I'm just repeatedly running `make test` and banging my head
against the idiosyncrasies of the test code (which is all written in Perl
and full of five- and six-parameter functions). I belatedly realize that
some of the test files are failing simply because they start with a cryptic
indication that says "I [plan](https://perldoc.perl.org/Test::More#I-love-it-when-a-plan-comes-together)
to run exactly 95 test cases," but I've added extra tests, and this makes
the plan fail.

Around 90 minutes, I call it a day. Some of the binary-protocol Perl tests
are still failing; but I'm confident that that's a problem with my tests,
not with the server code itself. Here's the secret downside to "just copy
and paste and change the names": The Perl tests for `incr` are basically
of the form

    initialize num to zero
    check that "incr num 1" returns 1
    check that "incr num 1" returns 2
    check that "incr num 1" returns 3

(obfuscated through a number of layers of indirection), so when I do
the obvious thing for multiplication, it comes out like

    initialize num to zero
    check that "mult num 1" returns 0
    check that "mult num 1" returns 0
    check that "mult num 1" returns 0

That's a pretty bad test. Now, if making good tests was the _point_
of this programming challenge, I'd spend the extra hour on it. Or if
I were doing this to get a job, instead of for a blog post, I'd spend
the extra hour on it. But for this blog post? I call it a day.


## Conclusion

I like this programming challenge because it's a microcosm of what
most real-world programming is like. When you're maintaining a large
codebase, there are always going to be codepaths you don't fully
understand, idioms that feel unnecessary, and masses of code that
can feel hard to get a foothold in.

This challenge is particularly well calibrated for an interview because
there is only one correct answer: "change `bool incr` to `int opcode`"
(or anything isomorphic to that). The codebase and problem statement
together very clearly imply that there are currently _two_ arithmetic
opcodes, and your job is to extend that to _three_ arithmetic opcodes.

Imagine how much more open-ended the problem would be if
memcached didn't have a `decr` command!
Suppose `process_arithmetic_command` had been named
`process_increment_command`, and `add_delta` didn't take `bool incr`
as a parameter, and so on. Then the candidate would have to make
a bigger creative decision: _add_ that parameter (in which position?),
or _clone_ an entire codepath (starting at what level?). Cloning even
one of these functions is probably suboptimal, but I might spend
twenty minutes before realizing that.

The problem as presented is well crafted to steer qualified
candidates right onto the happy path, while weeding out a whole
category of unqualified candidates. Basically, this question
is to software engineers as
[FizzBuzz](https://imranontech.com/2007/01/24/using-fizzbuzz-to-find-developers-who-grok-coding/)
is to programmers. And it's fun, too!

So there you have it: the best engineering interview question
I've ever gotten.
