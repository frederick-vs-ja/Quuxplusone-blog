---
layout: post
title: "Some St. Patrick's Day math"
date: 2024-03-17 00:01:00 +0000
tags:
  celebration-of-mind
  clement-wood
  litclub
  math
---

At today's _Gathering 4 Gardner_ social, Colm Mulcahy presented on two Irish figures
in recreational mathematics with whom Martin Gardner corresponded: Victor Meally and
Owen O'Shea. Owen O'Shea is the natural successor to Gardner's "Professor I.J. Matrix" as
a prolific generator of numerological coincidences — see for example
[_The Magic Numbers of the Professor_](https://amzn.to/3Ppt43I) (2007).
Victor Meally shows up occasionally in Gardner's _Mathematical Games_ columns,
and also in "Problem 3.14" (appropriate for Pi Day!) in
[_The Colossal Book of Short Puzzles and Problems_](https://archive.org/details/B-001-001-266/page/64/mode/2up) (2006):

> One of the satisfactions of recreational mathematics comes from finding better
> solutions for problems thought to have been already solved in the best possible way.
> Consider the following digital problem that appears as Number 81 in Henry Ernest
> Dudeney's _Amusements in Mathematics_. (There is a Dover reprint of this 1917 book.)
> Nine digits (0 is excluded) are arranged in two groups. On the left a
> three-digit number is to be multiplied by a two-digit number.
> On the right both numbers have two digits each:
>
> $$158\times 23 = 79\times 46$$
>
> In each case the product is the same: 3,634. How, Dudeney asked, can the same
> nine digits be arranged in the same pattern to produce as large a product as
> possible, and a product that is identical in both cases? Dudeney’s answer,
> which he said “is not to be found without the exercise of some judgment and
> patience,” was 5,568:
>
> $$174\times 32 = 96\times 58$$
>
> Victor Meally of Dublin County in Ireland later greatly improved on Dudeney's
> answer with 7,008:
>
> $$584\times 12 = 96\times 73$$
>
> This remained the record until a Japanese reader found an even better solution.
> It is believed, although it has not yet been proved, to give the highest possible product.
> Can you find it without the aid of a computer?

With the aid of a computer ([code](/blog/code/2024-03-17-digit-product.py)), it's easy to confirm that
that Japanese reader's solution is indeed the best of the 11 basic solutions
(and Meally's the runner-up):

    134 * 29 = 58 * 67   = 3386
    158 * 23 = 46 * 79   = 3634
    138 * 27 = 54 * 69   = 3726
    174 * 23 = 58 * 69   = 4002
    146 * 29 = 58 * 73   = 4234
    259 * 18 = 63 * 74   = 4662
    186 * 27 = 54 * 93   = 5022
    158 * 32 = 64 * 79   = 5056
    174 * 32 = 58 * 96   = 5568
    584 * 12 = 73 * 96   = 7008
    532 * 14 = 76 * 98   = 7448

But now consider the ruminations of another Irishman, James Joyce's Leopold Bloom, who
in [Chapter 17 of _Ulysses_](https://archive.org/details/ulysses0000joyc/page/644/mode/2up)
recounts how he became aware of

> the existence of a number computed to a relative degree of accuracy to be of
> such magnitude and of so many places, e.g., the 9th power of the 9th power of 9,
> that [...] 33 closely printed volumes of 1000 pages each of innumerable quires
> and reams of India paper would have to be requisitioned in order to contain the
> complete tale of its printed integers [...] the nucleus of the nebula of every
> digit of every series containing succinctly the potentiality of being raised
> to the utmost kinetic elaboration of any power of any of its powers.

There's [some confusion](https://thatsmaths.com/2013/06/13/joyces-number/) as to what
number Joyce was really talking about (if any); but the mathematical community has
apparently settled on $$9^{9^9}$$, as seen in e.g. the "_Ulysses_ sequence" [OEIS A054382](https://oeis.org/A054382):
$$\lceil\log_{10} 1^{1^1}\rceil, \lceil\log_{10} 2^{2^2}\rceil, \lceil\log_{10} 3^{3^3}\rceil,\ldots$$

The ninth element of this sequence — the number of decimal digits in $$9^{9^9}$$ — is
$$369\,693\,100$$. If "closely printed" at the resolution of
[_A Million Random Digits with 100,000 Normal Deviates_](https://en.wikipedia.org/wiki/A_Million_Random_Digits_with_100,000_Normal_Deviates),
printed by the RAND Corporation (no relation) in 1955, the recording of the entire
expansion of $$9^{9^9}$$ would take up 148 thousand-page volumes. (Or, if you postulate
a thousand _physical_ pages, each printed on both sides: 74 volumes.)

Suppose we allow solutions of Dudeney's problem to contain powers: not merely
$$abc\times de = gh\times ij$$, but for example $$ab^c\times d^e = g^h\times i^j$$.
Then there are five more basic solutions possible, the largest of which is

$$
    48^3 \times 9^1 = 2^7 \times 6^5
$$

The solutions are:

    329 * 8^1 = 47 * 56     = 2632
    574 * 9^1 = 63 * 82     = 5166
    49^2 * 8^1 = 56 * 7^3   = 19208
    1^67 * 8^5 = 2^9 * 4^3  = 32768  (also with 1^76)
    48^3 * 9^1 = 2^7 * 6^5  = 995328

Clement Wood — compiler of
[_The Best Irish Jokes_](https://digitalcommons.usf.edu/cgi/viewcontent.cgi?article=1021&context=irish_publications) (1926) —
asserts, in the same _Book of Mathematical Oddities_ (1927) which we previously
mined in ["Mathematical golf"](/blog/2023/03/23/mathematical-golf/) (2023-03-23),
that there are only two solutions to the double equality

    29 * 6 = 58 * 3 = 174
    39 * 4 = 78 * 2 = 156

Wood is correct, and the administration of powers produces no further solutions to that puzzle.
