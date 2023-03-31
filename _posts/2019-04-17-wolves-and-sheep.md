---
layout: post
title: "Puzzle: Wolves and sheep"
date: 2019-04-17 00:01:00 +0000
tags:
  math
  puzzles
---

Here's a new integer sequence that does not appear to be in the OEIS yet.
(UPDATE, 2023-03-31: This is now [OEIS A361928](https://oeis.org/A361928)!)

    d=       1  2  3  4  5  6  ...
          0
    n=1   0  0
    n=2   0  1  0
    n=3   0  2  2  0
    n=4   0  2  3  3  0
    n=5   0  3  4  4  4  0
    n=6   0  3  5  5  5  5  0
    n=7   0  3  6  6  6  6  6  0
    n=8   0  3  6  7  7  7  7  7  0
    n=9   0  4  7  8  8  8  8  8  8  0
    n=10  0  4  7  9  9  9  9  9  9  9  0
    n=11  0  4  8 10 10 10 10 10 10 10 10  0
    n=12  0  4  8 11 11 11 11 11 11 11 11 11  0
    n=13  0  4  .  . 12 12 12 12 12 12 12 12 12  0
    n=14  0  4  .  . 13 13 13 13 13 13 13 13 13 13  0
    n=15  0  4  .  .  . 14 14 14 14 14 14 14 14 14 14  0
    n=16  0  4  .  .  . 15 15 15 15 15 15 15 15 15 15 15  0
    n=17  0  5  .  .  .  . 16 16 16 16 16 16 16 16 16 16 16  0
    n=18  0  5  .  .  .  . 17 17 17 17 17 17 17 17 17 17 17 17  0
    n=19  0  5  .  .  .  .  . 18 18 18 18 18 18 18 18 18 18 18 18  0
    n=20  0  5  .  .  .  .  . 19 19 19 19 19 19 19 19 19 19 19 19 19  0


I admit that my OEIS-fu isn't very good. I've seen that when there's a
triangular sequence like this, it'll generally be entered into OEIS in row-major
order, i.e. $$1,2,2,2,3,3,3,4,4,4,3,5,5,5,5,3,6,6,6,6,6,3,6,7,$$—.

However, in this particular sequence, the rightmost edge is uninteresting (it's
just $$n-1$$), and so conceivably the sequence might be entered into OEIS sans that
edge: $$2,2,3,3,4,4,3,5,5,5,3,6,6,6,6,3,6,7,$$—. Or it might be entered including
one or more of the surrounding zeroes. Or some other more radical serialization,
such as $$1,2,2,2,3,3,3,4,3,3,5,4,3,6,5,4,4,6,6,5,4,7,7,6,5,$$—.

I wonder whether the OEIS's search function automatically looks for such variations
on a sequence being searched for, and how much work it would be to do so.
(UPDATE: It does not automatically look for such variations.)

----

The sequence above is defined as the solution to the following puzzle for various
values of $$(n,d)$$. Paraphrasing
[Jyotish Robin on Puzzling StackExchange](https://puzzling.stackexchange.com/questions/81737/):

> You have $$n$$ sheep. Unfortunately, you have been informed that exactly $$d$$
> of these sheep are really wolves in disguise. You have at your disposal a blood test
> that can reliably detect wolf DNA: given a vial of blood from any number of subject
> animals, a single test will tell you whether all of the subjects were innocent sheep
> or (vice versa) whether at least one subject was a wolf.
>
> The testing lab is in a distant city; therefore you must collect all your blood samples
> before you have learned any of the results. You cannot use the result of one test to
> inform your strategy for the other tests. Also, your testing strategy must have a 100%
> success rate at identifying all $$d$$ wolves; "99% probability of success" is not good
> enough for this puzzle.
>
> How can you minimize the number of tests required?

The first really interesting case is $$(n,d,t)=(8,2,6)$$. Suppose you have eight sheep, and
you know that two of them are wolves. Certainly you could find the wolves in seven tests:
you'd just test seven of your animals individually and then use the results to deduce the
species of the eighth. But how can you find the two wolves in _fewer_ than seven tests?

I have a brute-force solver [on GitHub](https://github.com/Quuxplusone/wolves-and-sheep).
It finds the answers up to $$n=9$$ pretty quickly; has some slowdown on $$n=10$$;
and takes quite a while on $$n=11$$. It can produce a solution for $$(10,2,7)$$ in
about 5 seconds, and for $$(11,2,8)$$ in just under a minute.

----

UPDATE, 2020-08-11: This problem is also commonly phrased in terms of $$n$$ bottles of wine,
$$d$$ of which are poisoned; it is generally known as "non-adaptive group testing."
This blog post used to refer to $$(n,k,t)$$; I've updated it to refer to $$(n,d,t)$$ instead.

Whereas our wolves-and-sheep problem was phrased in terms of minimizing the number of tests $$t$$
for a given $$(n,d)$$ ("To identify 2 wolves among 1000 sheep, you need no more than 27 tests"),
poisoned-wine solutions are often phrased in terms of maximizing the number of sheep $$n$$
that can be identified for a given $$(d,t)$$ ("Given 2 wolves and exactly 27 tests,
you can handle a flock of no less than 1065 sheep").

See also:

* ["Wolves and Sheep, with tables"](/blog/2020/01/10/wolves-and-sheep-with-tables) (2020-01-10)
