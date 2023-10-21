---
layout: post
title: "Happy 109th birthday, Martin Gardner!"
date: 2023-10-21 00:01:00 +0000
tags:
  celebration-of-mind
  math
  puzzles
---

Today Martin Gardner — born October 21, 1914 — celebrates his 109th birthday (_in absentia_).
His creation Dr. Matrix (who spent some time right up the road from Gardner and myself,
in Sing Sing — see Gardner's [_Mathematical Games_ column of January 1963](https://www-jstor-org.wikipedialibrary.idm.oclc.org/stable/24936434))
might have observed that Gardner's birth date 10/21 is made up of two products of the first
four primes:

$$
    \begin{aligned}
    10 &= 2\times 5 \\
    21 &= 3\times 7
    \end{aligned}
$$

and this year his age (109) is also a prime — but 2023 is not a prime; it's the product of 7 and 17².
Gardner's age and the current year will both be prime for the next time on his 113th birthday, in 2027;
after that it will happen only four more times in this century.

[Kronecker (1901)](https://archive.org/details/vorlesungenberz00krongoog/page/68/mode/1up) conjectured
that there will _never_ come a time when we cannot look forward to the next time Gardner's age and
the current year will both be prime; and in fact that that would be true no matter what year Gardner
was born in, as long as it was even. For example, if he were born in A.D. 2, we'd have the
[twin primes conjecture](https://mathworld.wolfram.com/TwinPrimeConjecture.html). Kronecker's conjecture
is itself a special case of the [prime patterns conjecture](https://mathworld.wolfram.com/k-TupleConjecture.html).

---

[Gardner's column of May 1959](https://www-jstor-org.wikipedialibrary.idm.oclc.org/stable/24940308)
presents the following puzzle:

> Mr. Smith has two children. At least one of them is a boy. What is the probability that the other child is a boy?

Leaving aside any similarity between this Mr. Smith's particular family and
[Portia's caskets](/blog/2018/07/06/portias-caskets-puzzle/), the answer is
clearly "One in three." If we assume Smith was selected at random from a large
ordinary population of people with two children, then with equal probabilities
Smith has two boys, or a boy and a girl, or a girl and a boy (but definitely not
two girls); thus the chance that both children are boys is one in three.

[An interesting variation](https://math.stackexchange.com/questions/4400/boy-born-on-a-tuesday-is-it-just-a-language-trick) asks:

> Mr. Smith has two children. At least one of them is a boy born on a Tuesday. What is the probability that the other child is a boy?

With equal probabilities Smith has a boy born on Tuesday and another boy (7 cases), a boy _not_ born on Tuesday
and a boy born on Tuesday (6 cases), a boy born on Tuesday and a girl (7 cases), or a girl and a boy born on Tuesday
(7 cases); the probability that he has two boys is thus 13 in 27, or about 0.48.

Puzzles in this genre are often controversial for the same reason as the Portia's Caskets puzzle:
Often the puzzle will be phrased something like,

> Mr. Smith says, "I have two children, at least one of whom is a boy." What is the probability that the other child is a boy?

In this phrasing, of course we have to weigh the psychological likelihood that Mr. Smith would say such an odd thing
if he really did have two boys (or for that matter if he didn't) — not to mention the possibility that Smith
might be lying entirely! Does he have _any_ children? Is Smith even his real name?

We might cross-breed the Smith puzzle with the [Monty Hall problem](https://en.wikipedia.org/wiki/Monty_Hall_problem):

> Mr. Smith has three children, at least two of whom are boys. He secretly places his three
> children behind three doors, and invites you to choose one of the doors.
> After you make your choice, rather than open your door, Smith will open one of the other doors
> to reveal a boy (and in fact he does exactly that).
> What's the probability that the child behind the third door is a boy?

Sometimes it helps to _reductio ad absurdam_ the puzzle. In this case I feel like it
might make matters _more_ confusing!

> Mr. Smith has 100 children, at least 99 of whom are boys. He secretly places his 100
> children behind 100 doors, and invites you to choose one of the doors.
> After you make your choice, rather than open your door, Smith will open 98 of the other doors
> to reveal 98 boys (and in fact he does exactly that).
> What's the probability that the child behind the third door is also a boy?
