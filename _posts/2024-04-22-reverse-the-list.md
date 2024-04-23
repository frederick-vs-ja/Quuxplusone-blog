---
layout: post
title: "Reverse the List of Integers"
date: 2024-04-22 00:01:00 +0000
tags:
  math
  puzzles
---

On 2024-04-09, Alexandre Muñiz [posted to Mathstodon](https://mathstodon.xyz/@two_star/112242224494626411)
a new mathematical recreation he calls "Reverse the List of Integers."
(Also on Hacker News [here](https://news.ycombinator.com/item?id=40010066).)
The game works like this:

- Start with a list of distinct positive integers, e.g. (10 5 3).
    Your goal is to reverse the list, via "moves" of these two kinds:

- Split a single number into two parts that sum to the whole; e.g. (10 5 4) could
    become (7 3 5 4) or (10 2 3 4).

- Combine two adjacent numbers into their sum; e.g. (7 3 5 4) could
    become (7 8 4) or (7 3 9).

- Throughout, it is forbidden to create a number greater than the _original_ list's maximum element.
    For example, if we're trying to reverse (10 5 4), then it's okay for
    (7 5 3 4) to become (7 8 4) but it's not okay for it to become (12 3 4)
    because 12 is greater than the original list's maximum of 10.

- Finally, throughout, the list's elements must remain distinct; e.g.
    (7 5 3 4) cannot become (7 5 7) nor (7 2 3 3 4).

Alexandre asks:

> What are good algorithms, or even general strategies, for solving these?
>
> For a given $$n$$, there must be some [list] where $$n$$ is the largest number in the list,
> and the number of moves required to solve the puzzle is maximized. What does the sequence
> of maximal required moves look like as a function of $$n$$? What do the "hardest" puzzles
> look like? Is there a way to determine either without using brute force?

---

Hacker News user "Someone" [describes](https://news.ycombinator.com/item?id=40016067) a good
physical model of the game: Collect sticks of lengths 1 through $$n$$. Place some subset of them
end-to-end in the "solving gutter"; place the rest loose in the "unused gutter." Your possible
moves then correspond to replacing two adjacent sticks in the solving gutter with one unused
stick of the same length from the unused gutter, or replacing one stick in the solving gutter
with any two sticks from the unused gutter.

![Visualization of reversing (6 1 3 2)](/blog/images/2024-04-22-gutters.svg)

Notice that the sum of the list elements remains constant throughout; and
therefore so does the sum of the missing elements (the sticks in the unused gutter).
This invariant is useful in some of the proofs below; in particular, notice that
we cannot ever split an element $$a$$ unless the sum of the sticks in the unused gutter
is at least $$a$$. If, for some initial list, the sum of the unused elements is less
than $$n-1$$, then neither $$n-1$$ nor $$n$$ can be split; thus they cannot change
places; and thus the list will be unsolvable.

---

It seems generally useful to classify the possible starting arrangements by their
high value $$n$$ and their initial length $$k$$. (For example, (10 3 5) has high value 10
and length 3.)

Tomas Rokicki has already done some investigating and OEIS-ifying.
OEIS [A372008](https://oeis.org/A372008) gives the worst-case number of moves
needed to solve any solvable list with high value `n`.
Its entries are the maxima of each row of this triangle $$M(n,k)$$, whose entries indicate
the worst-case number of moves to solve any solvable list with high value $$n$$ and length $$k$$:

    n=1:  0
    n=2:  0 -
    n=3:  0 - -
    n=4:  0 - -  -
    n=5:  0 - -  -  -
    n=6:  0 6 14 6  -  -
    n=7:  0 4 12 24 26 -  -
    n=8:  0 4 14 32 64 74 -   -
    n=9:  0 4 8  18 66 76 86  -    -
    n=10: 0 4 8  14 20 88 124 126 36  -
    n=11: 0 4 8  12 16 26 ?   ?    ?  -  -
    n=12: 0 4 8  12 16 ?  ?   ?    ?  ?  -  -
    n=13: 0 4 8  10 14 ?  ?   ?    ?  ?  ?  -  -
    n=14: 0 4 8  12 16
    n=15: 0 4 8  10
    n=16: 0 4 8  12
    n=17: 0 4 8  10
    n=18: 0 4 8  12
    n=19: 0 4 8  10
    n=20: 0 4 8

Meanwhile the count $$C(n,k)$$ of solvable lists with high value $$n$$ and length $$k$$ is:

    n=1:  1
    n=2:  1  0
    n=3:  1  0    0
    n=4:  1  0    0     0
    n=5:  1  0    0     0     0
    n=6:  1  8   26    12     0      0
    n=7:  1 12   82   294   244      0      0
    n=8:  1 14  126   760  2316   1846      0      0
    n=9:  1 16  168  1344  8238  31678  47164      0      0
    n=10: 1 18  216  2016 15098  89078 336154 480598   2640  0
    n=11: 1 20  270  2880 25200 181308      ?      ?      ?  0  0
    n=12: 1 22  330  3960 39600      ?      ?      ?      ?  ?  0  0
    n=13: 1 24  396  5280 59400      ?      ?      ?      ?  ?  ?  ?  0
    n=14: 1 26  468  6864 85800      ?      ?      ?      ?  ?  ?  ?  ?  0
    n=15: 1 28  546  8736     ?      ?      ?      ?      ?  ?  ?  ?  ?  0  0
    n=16: 1 30  630 10920     ?      ?      ?      ?      ?  ?  ?  ?  ?  ?  0  0 
    n=17: 1 32  720 13440
    n=18: 1 34  816 16320
    n=19: 1 36  918 19584
    n=20: 1 38 1026

Notice that:

- The total number of lists (solvable or not) with high value $$n$$ and length $$k$$ is $$\mathrm{Total}(n,k) = k!{ {n-1}\choose{k-1}}$$.

- Invariably $$C(n,1)=1$$ and $$M(n,1)=0$$.

- For $$n\geq 2$$ we have $$C(n,n)=0$$, since if the starting list already contains all $$n$$ possible numbers, there are no legal moves.

- Consider a list of length $$k=n-1$$. Only one number $$a < n$$ is missing from the initial list; this means we'll never
    be able to split the high value $$n$$ itself; we can only manipulate the elements to the left of $$n$$
    and the elements to the right of $$n$$. So, for this list to be solvable, the sum of the elements to the left of $$n$$
    must equal the sum of the elements to the right of $$n$$. So the missing element must have the same parity as $$n(n+1)/2$$.
    Now, if $$n-1$$ is on the left side initially, we'll certainly need to split it and reconstitute it on the right side,
    which means the sum of the missing numbers must be at least $$n-1$$; but that's impossible, because only $$a$$ is missing.
    So the missing element $$a$$ must be $$n-1$$, and it must have the same parity as $$n(n+1)/2$$ so that we can divide the
    remainder evenly between the left and right sides.
    According to this logic, $$C(n,n-1)$$ is certainly zero for $$n\in\lbrace11,12,15,16,19,20,\ldots\rbrace$$.
    On the other hand, it _can_ be nonzero; for example $$C(10,9)=2640$$.

- It seems intuitively plausible that $$\forall k\exists m\forall n > m: C(n,k)=\mathrm{Total}(n,k)$$.

- For $$n\geq 7$$, we have $$C(n,2)=\mathrm{Total}(n,2)$$ and $$M(n,2)=4$$.
    A proof of this is sketched by Mastodon user "SevenNinths" [here](https://pawb.fun/@SevenNinths/112255955334040349).

- For $$n\geq 9$$, we seem to have $$C(n,3)=\mathrm{Total}(n,3)$$ and $$M(n,3)=8$$.

- For $$n\geq 9$$, we seem to have $$C(n,4)=\mathrm{Total}(n,4)$$; but thus far $$M(n,4)$$ fluctuates between 10 and 12.
    Will it settle down to 12 (suggesting that $$\forall k\exists m\forall n > m: M(n,k)=4(k-1)$$), or is something
    more complicated going on here?

Mastodon user "SevenNinths" [sketches a constructive proof](https://pawb.fun/@SevenNinths/112255955334040349) that
$$\forall n\geq 7: M(n,2)=4$$, in the form of a foolproof algorithm for reversing any two-element list with $$n\geq 7$$.
This intuitively suggests that there ought to be a foolproof algorithm for three-element lists, one for four-element lists,
and so on. And then: Observe that SevenNinths' algorithm keeps the length of all intermediate lists to 4 or less, even
for extremely large $$n$$. This intuitively suggests that there ought to be a sufficient length $$L(n,k) < n$$ such that
every solvable list with high value $$n$$ and initial length $$k$$ can be solved without ever creating a list longer
than $$L(n,k)$$. An oracle for $$L(n,k)$$ would drastically speed up a brute-force solver by cutting down the search space.

The sufficient intermediate list length $$L(n,k)$$ for solvable lists with high value $$n$$ and length $$k$$ is:

    n=1:  1
    n=2:  1 -
    n=3:  1 - -
    n=4:  1 - - -
    n=5:  1 - - - -
    n=6:  1 4 4 4 - -
    n=7:  1 4 5 5 5 - -
    n=8:  1 4 5 6 7 7 - -
    n=9:  1 4 5 7 7 8 8 - -
    n=10: 1 4 5 7 8 9 9 9 9 -
    n=11: 1 4 5 7 8 9 ? ? ? - -
    n=12: 1 4 5 6 8 ? ? ? ? ? - -
    n=13: 1 4 5 7 8 ? ? ? ? ? ? ? -
    n=14: 1 4 5 6 7 ? ? ? ? ? ? ? ? -
    n=15: 1 4 5 7 ? ? ? ? ? ? ? ? ? - -
    n=16: 1 4 5 6 ? ? ? ? ? ? ? ? ? ? - -
    n=17: 1 4 5 7 ? ? ? ? ? ? ? ? ? ? ? ? -
    n=18: 1 4 5 6 ? ? ? ? ? ? ? ? ? ? ? ? ? -
    n=19: 1 4 5 7 ? ? ? ? ? ? ? ? ? ? ? ? ? - -
    n=20: 1 4 5 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? - -

The C++17 code that produced these tables is [here](/blog/code/2024-04-22-solver.cpp).
