---
layout: post
title: "Nash equilibria in Ballmer's binary-search interview game"
date: 2024-09-04 00:01:00 +0000
tags:
  math
  puzzles
---

Yesterday John Graham-Cumming posted about
["Steve Ballmer's incorrect binary search interview question,"](https://blog.jgc.org/2024/09/steve-ballmers-binary-search-interview.html)
and in [the Hacker News discussion](https://news.ycombinator.com/item?id=41434637), among the
predictable <i>"interview culture is flawed!!"</i> complaints, there was some discussion of
whether binary search was even the right way to solve Ballmer's puzzle. Here's a stab at
an answer.

Ballmer's puzzle boils down to:

> I'm thinking of a number between `1` and `100`. You make a series of guesses. After each guess
> I'll tell you whether my number is higher or lower. If you hit it on the first guess I'll give you $1000.
> On the second guess, $990. On the third guess, $980. And so on; until, if you hit it on your 100th guess,
> I'll give you only $10.
>
> What is the expected value, to you, of playing this game with me?

Here I've fiddled with the numbers to keep all the payouts positive and to help separate your possible
guesses (in `teletype font`) from payouts (preceded by a $dollar sign). In Ballmer's actual interview,
he was asking basically whether the expected value of this game is at least $950.

> HN rightly points out that the intangible value of getting to play such a game against Steve Ballmer
> is _very high_ regardless of how many dollars you expect to win or lose at it. But let's focus on the math.

Binary search seems like a decent way to go about tackling this game. If Ballmer is choosing his secret
number uniformly at random, then the expected value of the game is $952. But, as Ballmer points out
[in the linked video](https://www.youtube.com/watch?v=svCYbkS0Sjk&t=37s), if he _knows_ you're going
to do a plain old binary search, then he certainly won't ever choose `50` as his secret number. In fact,
he has no reason to choose `25` or `75` either. Or `12`, `13`, `37`, `38`, `62`, `63`, `87`, or `88`.
If Ballmer avoids those eleven numbers, and chooses uniformly at random from among the rest, that alone
is enough to lower the expected value of the game from $952 to about $949.55.

But of course if you know that Ballmer is going to do _that_, then you'll start your binary search at `49`
instead of `50`. And so on.
It turns out that this question is really about computing the game's [Nash equilibrium](https://en.wikipedia.org/wiki/Nash_equilibrium).

## The three-number version

Let's tackle a stripped-down version of the problem first.

Interviewer Alice will choose a number between `1` and `3`. Interviewee Bob will make a series
of guesses; after each guess, Alice will say "Higher" or "Lower" or "Hit."
The payout is $30 if Bob hits on the first guess; $20 on the second; or $10 on the third.

There are three pure strategies for Alice, and five pure strategies for Bob.
The payoff matrix is:

                            p   q   r
                            1   2   3
    ------------------------------------
    a "Guess 1, then 3"    30  10  20
    b "Guess 1, then 2"    30  20  10
    c "Guess 2, then win"  20  30  20
    d "Guess 3, then 2"    10  20  30
    e "Guess 3, then 1"    20  10  30

Alice is looking for a mixed strategy $$(p,q,r)$$ (with all three values between 0 and 1,
and $$r = 1-p-q$$) that minimizes the value of Bob's best response. Vice versa,
Bob is looking for a mixed strategy $$(a,b,c,d,e)$$ (with $$e=1-b-c-d$$) that maximizes
his expected value given Alice's best strategy.

Without loss of generality, we can assume by symmetry that $$a=e, b=d, p=r$$;
thus $$q=1-2p$$ and $$c=1-2a-2b$$.
Bob's expected payoff, for each of his three distinct strategies, is:

$$
\begin{align}
    E_a &= 30p+10q+20r = 50p+10(1-2p) = 10+30p \\
    E_b &= 30p+20q+10r = 40p+20(1-2p) = 20 \\
    E_c &= 20p+30q+20r = 40p+30(1-2p) = 30-20p
\end{align}
$$

We can plug in some values for $$p$$ (remember, that's Alice's mixed strategy's likelihood
of picking `1`, which is the same as its likelihood of picking `3`), to see how Bob's
pure strategies would fare in response.

- If Alice's $$p=0.5$$, then Bob's $$(E_a,E_b,E_c) = (25, 20, 20)$$. That is, if Alice commits
to a mixed strategy that chooses `1` or `3` with equal probability and never chooses `2`, then
Bob's best strategy is to guess `1`, then `3` (or vice versa) with equal probability. That'll
pay $25 per game.

- If Alice's $$p=0$$, then Bob's $$(E_a,E_b,E_c) = (10, 20, 30)$$. That is, if Alice always
picks good old `2`, then Bob's best strategy by far is binary search, because it'll always hit
on the first guess.

- If Alice's $$p=1/3$$, then Bob's $$(E_a,E_b,E_c) = (20, 20, 23.33)$$. That is, if Alice
picks each number equally often, then Bob's best strategy is to use binary search.

This is significant: Somewhere between $$p=0.5$$ and $$p=1/3$$, Bob's best strategy must _change_
from "guess the endpoints first" to "binary search." There will be some value of $$p$$ for which
it doesn't matter whether Bob does binary search or not; and _that_ will be our Nash equilibrium.
Iteratively search for that $$p$$, and you'll find:

- If Alice's $$p=0.4$$, then Bob's $$(E_a,E_b,E_c) = (22, 20, 22)$$. That is, if Alice
picks `2` with probability 0.2 and `1` or `3` with probability 0.4 each, then it doesn't
matter what Bob does. Well, except for strategy `b` (linear search) — that strategy is dominated
by the other two options.

What does this mean for our interview? If Alice chooses her optimal strategy $$p=0.4$$, then
it doesn't matter what Bob does, so he might as well always choose binary search, right?
Surely "always binary search" is the smart strategy?

Well, no; because as we've seen, if Alice *knows* that Bob is playing a pure binary-search
strategy, she'll switch to $$p=0.5$$ and reduce Bob's expected payout from $22 per game
to only $20 per game (because in that case Bob's first guess will *always* be `2`
and Alice's secret number will *never* be `2`).

So Bob is looking for a mixed strategy $$(a,b,c)$$ that maximizes the smaller of Alice's
expected values:

$$
\begin{align}
    E_p &= 30a+30b+20c+10d+20e = 50a+40b+20(1-2a-2b) = 20+10a \\
    E_q &= 10a+20b+30c+20d+10e = 20a+40b+30(1-2a-2b) = 30-40a-20b
\end{align}
$$

- If Bob's $$(a,b)=(0,0)$$, then Alice's $$(E_p,E_q)=(20,30)$$. That is, if Bob always does binary search,
then Alice's best strategy is to avoid `2`.

- If Bob's $$(a,b)=(.5,0)$$, then Alice's $$(E_p,E_q)=(25,10)$$. That is, if Bob always guesses the endpoints first,
then Alice's best strategy by far is to pick `2`, because it'll always pay out a mere $10.

- If Bob's $$(a,b)=(0,.5)$$, then Alice's $$(E_p,E_q)=(20,20)$$. That is, if Bob always guesses in ascending or descending order,
then it doesn't matter what Alice picks.

Somewhere in the two-dimensional space between $$(a,b)=(0,0)$$ and $$(a,b)=(.5,0)$$, we expect
that Alice's best strategy will change from "avoid `2`" to "pick `2`"; and that should be our Nash
equilibrium. That equilibrium is at:

- If Bob's $$(a,b)=(.2,0)$$, then Alice's $$(E_p,E_q)=(22,22)$$. That is, if Bob guesses `2` with probability 0.6,
and splits his other guesses evenly between `1` and `3`, then it doesn't matter what Alice does.

That seems right, because from both directions we ended up at the same result:
The expected value of the three-number game is $22. The equilibrium mixed strategies are:

> Alice: Choose `1`, `2`, or `3` with probabilities (.4, .2, .4).
>
> Bob: Guess `1`, `2`, or `3` with probabilities (.2, .6, .2); if you guess one endpoint
> and fail to hit, then follow up by guessing the other endpoint.

## The four-number version

Suppose Alice picks a number between `1` and `4`. Then
there are four pure strategies for Alice, and seven pure strategies for Bob
(up to symmetry; without loss of generality we can assume Bob guesses `1` or `2` first).
The payoff matrix is:

                            p   q   r   s
                            1   2   3   4
    --------------------------------------
    a "Guess 2, 3"         30  40  30  20
    b "Guess 2, 4"         30  40  20  30
    c "Guess 1, 2, 4"      40  30  10  20
    d "Guess 1, 3"         40  20  30  20
    e "Guess 1, 4, 2"      40  20  10  30
    f "Guess 1, 2, 3"      40  30  20  10
    g "Guess 1, 4, 3"      40  10  20  30

where again we can assume that $$p=s, q=r$$.

$$
\begin{align}
    E_a &= 30p+40q+30r+20s = 50p+70(0.5-p) = 35-20p \\
    E_b &= 30p+40q+20r+30s = 60p+60(0.5-p) = 30 \\
    E_c &= 40p+30q+10r+20s = 60p+40(0.5-p) = 20p+20 \\
    E_d &= 40p+20q+30r+20s = 60p+50(0.5-p) = 10p+25 \\
    E_e &= 40p+20q+10r+30s = 70p+30(0.5-p) = 40p+15 \\
    E_f &= 40p+30q+20r+10s = 50p+50(0.5-p) = 25 \\
    E_g &= 40p+10q+20r+30s = 70p+30(0.5-p) = 40p+15
\end{align}
$$

We see that regardless of $$p$$, Bob's strategy `b` (binary search) dominates `f` (linear search);
and `g` is equivalent to `e` (endpoints first, then the two middle numbers in either order).
Alice's best mixed strategy keeps the expected value of all Bob's strategies to no more than the (constant)
expected value of `b`:

- If Alice's $$p=.25$$, then Bob's $$(E_a,E_b,E_c,E_d,E_e)=(30,30,25,27.5,25)$$.
That is, if Alice chooses uniformly at random, then it doesn't matter what Bob does... except that
he ought to pick `2` or `3` first.

And then we flip it around and look for Bob's best mixed strategy:

$$
\begin{align}
    E_p &= (50a+60b+60c+60d+70(1-a-b-c-d))/2 = 35-10a-5b-5c-5d \\
    E_q &= (70a+60b+40c+50d+30(1-a-b-c-d))/2 = 15+20a+15b+5c+10d
\end{align}
$$

- If Bob's $$(a,b,c,d)=(.5,.5,0,0)$$, then Alice's $$(E_p,E_q)=(27.5, 32.5)$$.
That is, if Bob always picks `2` or `3` first, then Alice will do best to choose `1` or `4`
as her secret number.

- If Bob's $$(a,b,c,d)=(0,1,0,0)$$, then Alice's $$(E_p,E_q)=(30, 30)$$.
That is, if Bob always picks one of the middle numbers followed by one of the endpoints,
then it doesn't matter what Alice does.

So in the four-number version, unless I'm mistaken — which is quite possible! —
the Nash equilibrium is surprisingly conformist:

> Alice: Choose `1`, `2`, `3`, or `4` uniformly at random.
>
> Bob: Guess `2` or `3` uniformly at random; then, if you still have a meaningful
> choice to make, prefer to guess the endpoint rather than the other middle number.


## The five-number version

Suppose Alice picks a number between `1` and `5`. Then
there are five pure strategies for Alice, and seven pure strategies for Bob
(up to symmetry). The payoff matrix is:

                            p   q   r   s   t
                            1   2   3   4   5
    ------------------------------------------
    a "Guess 1,2,3,4"      50  40  30  20  10
    b "Guess 1,2,3,5"      50  40  30  10  20
    c "Guess 1,2,4"        50  40  20  30  20
    d "Guess 1,2,5,3"      50  40  20  10  30
    e "Guess 1,2,5,4"      50  40  10  20  30
    f "Guess 1,3,4"        50  30  40  30  20
    g "Guess 1,3,5"        50  30  40  20  30
    h "Guess 1,4,2"        50  30  20  40  30
    i "Guess 1,4,3"        50  20  30  40  30
    j "Guess 1,5,2,3"      50  30  20  10  40
    k "Guess 1,5,2,4"      50  30  10  20  40
    l "Guess 1,5,3"        50  20  30  20  40
    m "Guess 2,3,4"        40  50  40  30  20
    n "Guess 2,3,5"        40  50  40  20  30
    o "Guess 2,4"          40  50  30  40  30
    p "Guess 2,5,3"        40  50  30  20  40
    q "Guess 2,5,4"        40  50  20  30  40
    r "Guess 3,1,4"        40  30  50  40  30
    s "Guess 3,1,5"        40  30  50  30  40
    t "Guess 3,2,4"        30  40  50  40  30

(In this payoff matrix, for the first time, we have to describe _branching_ strategies.
For example strategy `r` means "Guess `3`; then, if Alice said ‘Lower’ guess `1` but
if she said ‘Higher’ guess `4`.”)

The expected values of Bob's pure strategies are as follows. `p` dominates `a` and `i`.
`n` dominates `f`. `o` dominates `b`. `q` dominates `c`, `d`, and `h`.

$$
\begin{align}
    E_e &= 50p+40q+10r+20s+30t = 80p + 60q + 10(1-2p-2q) = 10 + 60p + 40q \\
    E_g &= 50p+30q+40r+20s+30t = 80p + 50q + 40(1-2p-2q) = 40 - 30q \\
    E_j &= 50p+30q+20r+10s+40t = 90p + 40q + 20(1-2p-2q) = 20 + 50p \\
    E_k &= 50p+30q+10r+20s+40t = 90p + 50q + 10(1-2p-2q) = 10 + 70p + 30q \\
    E_l &= 50p+20q+30r+20s+40t = 90p + 40q + 30(1-2p-2q) = 30 + 30p - 20q \\
    E_m &= 40p+50q+40r+30s+20t = 60p + 80q + 40(1-2p-2q) = 40 - 20p \\
    E_n &= 40p+50q+40r+20s+30t = 70p + 70q + 40(1-2p-2q) = 40 - 10p - 10q \\
    E_o &= 40p+50q+30r+40s+30t = 70p + 90q + 30(1-2p-2q) = 30 + 10p + 30q \\
    E_p &= 40p+50q+30r+20s+40t = 80p + 70q + 30(1-2p-2q) = 30 + 20p + 10q \\
    E_q &= 40p+50q+20r+30s+40t = 80p + 80q + 20(1-2p-2q) = 20 + 40p + 40q \\
    E_r &= 40p+30q+50r+40s+30t = 70p + 70q + 50(1-2p-2q) = 50 - 30p - 30q \\
    E_s &= 40p+30q+50r+30s+40t = 80p + 60q + 50(1-2p-2q) = 50 - 20p - 40q \\
    E_t &= 30p+40q+50r+40s+30t = 60p + 80q + 50(1-2p-2q) = 50 - 40p - 20q
\end{align}
$$

- If Alice's $$(p,q)=(.5,0)$$, then Bob's $$E = (E_{e,g,j,k,l,m,n,o,p,q,r,s,t}) =$$ (40, 40, 45, 45, 45, 30, 35, 35, 40, 40, 35, 40, 30).
That is, if Alice commits to a mixed strategy that chooses `1` or `5` with equal probability,
then Bob's best strategy is to focus on those two numbers and choose strategy `j`, `k`, or `l`.

- If Alice's $$(p,q)=(.2,.2)$$, then Bob's $$E=$$ (30, 34, 30, 30, 32, 36, 36, 38, 36, 36, 38, 38, 38).
That is, if Alice chooses uniformly at random, then Bob will do well with `o`, `r`, `s`, or `t`.

- If Alice's $$(p,q)=(.375,.125)$$, then Bob's $$E=$$ (37.5, 36.25, 38.75, 40, 38.75, 32.5, 35, 37.5, 38.75, 40, 35, 37.5, 32.5).
That is, if Alice chooses `1` through `5` in the proportion 3:1:0:1:3, then Bob will do well with `k` or `q`.

- If Alice's $$(p,q)=(5/18, 1/6)$$, then Bob's $$E=$$ (33.33, 35, 33.88, 34.44, 35, 34.44, 35.55, 37.77, 37.22, 37.77, 36.66, 37.77, 35.55).
That is, Bob will do equally well with `o`, `q`, or `s`.

Meanwhile, from the other direction, Alice's expected payouts are:

$$
\begin{align}
  E_p &= (80e+80g+90j+90k+90l+60m+70n+70o+80p+80q+70r+80s+60t)/2 \\
  E_q &= (60e+50g+40j+50k+40l+80m+70n+90o+70p+80q+70r+60s+80t)/2 \\
  E_r &= (10e+40g+20j+10k+30l+40m+40n+30o+30p+20q+50r+50s+50t)
\end{align}
$$

- If Bob's strategy is 50% `o` and 50% `s`, then Alice's payouts are $$E = (E_{p,q,r}) = (37.5, 37.5, 40)$$,
meaning that Alice should avoid selecting `3` as her secret number, meaning in turn that Bob should counter by mixing in
a strategy that avoids starting with `3` — namely `q`. (Recall from above that Bob's best pure strategies in this case
are `o`, `q`, and `s`.)

- If Bob's strategy is 4/9 `o`, 4/9 `s`, and 1/9 `q`, then Alice's payouts are $$E = (37.77, 37.77, 37.77)$$.

So there's at least one Nash equilibrium where Alice chooses `1` through `5` in the proportion
5:3:2:3:5; Bob mixes strategies `o`, `q`, and `s` in the proportion 4:1:4; and the expected value of the game is 37.77 ("repeating, of course").
There could be other Nash equilibria, too, but by the definition of equilibrium, the expected value of the game
will be the same — 37.77 — at every equilibrium point.

## Keep going?

I've spent much too long on these manual computations already; but I'd certainly be interested to see someone
make better use of the computer to explore the same puzzle for 6, 7, 8,... numbers — perhaps all the way up to
Ballmer's 100-number puzzle. What is the expected value of the 100-number game?

> In the video interview, Ballmer claims that "one guy I interviewed at Purdue" instantly wrote down
> the expected value of the 100-number game. I suspect that if that did happen, the guy's answer was
> incorrect — but hey, maybe there's a trick I'm not seeing. If
> [Gauss could do it](https://hsm.stackexchange.com/questions/384/did-gauss-find-the-formula-for-123-ldotsn-2n-1n-in-elementary-school),
> maybe that guy could, too.

To sum up what I think we've computed in this blog post:

| Game      | Expected number of guesses | Expected value (out of max) | Alice's mixed strategy for choosing | Bob's mixed strategy for guessing |
|-----------|:-----|:-------------|-----------|:-----------------------------------------|:----------------------------------|
| 3 numbers | 1.8  | $22 ($30)    | 2:1:2     | 1:3:1, then guess an endpoint            |
| 4 numbers | 2    | $30 ($40)    | 1:1:1:1   | 0:1:1:0, then guess an endpoint          |
| 5 numbers | 2.22 | $37.77 ($50) | 5:3:2:3:5 | 4/9 guess `3` then an endpoint; 4/9 guess `2` or `4` then binary search; 1/9 guess `2` or `4` then linear-search from the endpoint |

## Update, 2024-09-06

Konstantin Gukov, in ["Steve Ballmer was wrong"](https://gukov.dev/puzzles/math/2024/09/05/steve-ballmer-was-wrong.html) (2024-09-05),
shows that we can use `scipy.optimize.linprog` to do the icky tedious algebra above. All we have to provide is the payoff matrix —
with a column for each of Alice's possible choices and a row for each of Bob's possible guessing strategies — and it'll spit out an
equilibrium position *for that particular set of strategies.* But we can increase the value of the game to Bob by increasing the
range of Bob's imagination. Konstantin started out with a range of strategies that let Bob hit in 5.93 guesses on average;
then [I improved that](https://github.com/gukoff/ballmer_puzzle/pull/1) to 5.8535 guesses; then
[again](https://github.com/gukoff/ballmer_puzzle/pull/2) to 5.8333 guesses, for a profit of 16.67 cents per game.
That best-so-far strategy is a mixture of 83 different pure strategies.

Meanwhile, [Bo Waggoner](https://bowaggoner.com/blahg/2024/09-06-adversarial-binary-search/) claims a proof
(although I don't understand the math involved) that the true equilibrium is somewhere between
5.80371 guesses and 5.81087 guesses.

Meanwhile, [Eric Farmer](https://possiblywrong.wordpress.com/2024/09/04/analysis-of-adversarial-binary-search-game/)
also computes some expected values — some exactly, some approximately.
