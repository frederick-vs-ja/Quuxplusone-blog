---
layout: post
title: 'The algebraic structure of _Infinite Craft_'
date: 2024-03-03 00:01:00 +0000
tags:
  knuth
  math
  web
---

Last month I [wrote](/blog/2024/02/08/infinite-craft/) about Neal Agarwal's web game [_Infinite Craft_](https://neal.fun/infinite-craft/).
Tom Fang wrote to tell me he's created [a dictionary](https://szdytom.github.io/infinite-craft-dictionary/)
of _Infinite Craft_ elements, along with their uses and recipes. This got me thinking about the
game's mathematical structure.

By "mathematical structure," I mean something like how we make recipes and the metrics by which we
might compare one recipe to another. For example, if our goal is to make Sandwich, we could do it like this:

    Wave = Water + Wind
    Steam = Fire + Water
    Plant = Earth + Water
    Sand = Earth + Wave
    Tea = Plant + Steam
    Sandwich = Sand + Tea

Or like this:

    Wave = Water + Wind
    Sand = Earth + Wave
    Glass = Fire + Sand
    Wine = Glass + Water
    Sandwich = Sand + Wine

If we diagram these recipes, we find that the first one is _shallower_, in the sense that we combine only the
primitive elements, elements made _from_ the primitives (Wave, Steam, Plant), and elements made from _those_
elements (Sand, Tea). But the second one is _terser_, in the sense that it is five lines long instead of six.

At first glance, the world of _Infinite Craft_ forms a [directed hypergraph](https://en.wikipedia.org/wiki/Hypergraph)
with a lot of structure: each directed hyperedge connects a set of one or two vertices to a set of exactly one vertex.
But a recipe isn't merely a "path" on that hypergraph!
Mathematicians define the hypergraph analogue of a "path" as a set of incident hyperedges — "incidence"
meaning that the edges share _at least one_ vertex. In this sense there is a "path" from the starting elements
to Sandwich that consists of only a single hyperedge; namely,

    Sandwich = Wind + Grilled Cheese

That definition of "path" doesn't help us.

[I asked MathOverflow](https://mathoverflow.net/questions/466176/what-is-the-proper-name-for-this-tersest-path-problem-in-infinite-craft),
and they pointed me to another example of the same problem: [addition chains](https://en.wikipedia.org/wiki/Addition_chain).
An "addition chain" for an integer $$n$$ is a sequence starting with 1 and ending with $$n$$, such that each
element in the sequence is the sum of exactly two previous elements. For example,
we might make 31 in any of these three ways:

    2 = 1 + 1              2 = 1 + 1              2 = 1 + 1
    3 = 1 + 2              3 = 1 + 2              3 = 1 + 2
    6 = 3 + 3              4 = 2 + 2              6 = 3 + 3
    7 = 1 + 6              8 = 4 + 4              12 = 6 + 6
    14 = 7 + 7             12 = 8 + 4             14 = 2 + 12
    15 = 1 + 14            16 = 8 + 8             17 = 3 + 14
    30 = 15 + 15           28 = 12 + 16           31 = 14 + 17
    31 = 1 + 30            31 = 3 + 28

The middle chain is _shallowest_, but the right-hand one is _tersest_.

Addition chains are idiomatically written as just an increasing sequence of integers: (1 2 3 6 12 14 17 31).
We don't need to specify how each integer (say, 17) is constructed from the preceding elements, because it's
obvious. We could represent _Infinite Craft_ recipes just as concisely — (Wave Sand Glass Wine Sandwich) — but
that wouldn't be very reader-friendly because it's not obvious _which_ two preceding elements combine to make,
say, Wine.

----

Finding the tersest addition chain is directly relevant to the world of computer programming.
Suppose we want to calculate the 31st power of an unknown number in register `A`,
using only multiplication. Then we can do any of:

    mul A, A, B  # A^2     mul A, A, B  # A^2     mul A, A, B  # A^2
    mul A, B, C  # A^3     mul A, B, C  # A^3     mul A, B, C  # A^3
    mul C, C, D  # A^6     mul B, B, D  # A^4     mul C, C, D  # A^6
    mul A, C, E  # A^7     mul D, D, E  # A^8     mul D, D, E  # A^12
    mul E, E, F  # A^14    mul D, E, F  # A^12    mul B, E, F  # A^14
    mul A, F, G  # A^15    mul E, E, G  # A^16    mul C, F, G  # A^17
    mul G, G, H  # A^30    mul F, G, H  # A^28    mul F, G, R  # A^31
    mul A, H, R  # A^31    mul C, H, R  # A^31

Our "shallowness" metric translates into a measure of the [data dependencies](https://en.wikipedia.org/wiki/Data_dependency)
involved in these computations. The middle program, being the shallowest,
is also the _fastest_ on any machine with at least two multiplier units.

Another practically relevant metric for the "goodness" of a chain is its _width_: the number of
registers it uses in its most optimal [coloring](https://en.wikipedia.org/wiki/Register_allocation#Graph-coloring_allocation).
The left-hand recipe above is the _narrowest_, with width 2, whereas the others have width 3:

    mul A, A, B  # A^2     mul A, A, B  # A^2     mul A, A, B  # A^2
    mul A, B, B  # A^3     mul A, B, A  # A^3     mul A, B, A  # A^3
    mul B, B, B  # A^6     mul B, B, C  # A^4     mul A, A, C  # A^6
    mul A, B, B  # A^7     mul C, C, B  # A^8     mul C, C, C  # A^12
    mul B, B, B  # A^14    mul C, B, C  # A^12    mul B, C, C  # A^14
    mul A, B, B  # A^15    mul B, B, B  # A^16    mul A, C, A  # A^17
    mul B, B, B  # A^30    mul C, B, B  # A^28    mul C, A, R  # A^31
    mul A, B, R  # A^31    mul A, B, R  # A^31

The left-hand recipe corresponds to [Russian peasant multiplication](https://en.wikipedia.org/wiki/Ancient_Egyptian_multiplication),
which always generates an addition chain of width 2. For in-depth coverage of various algorithms to generate
addition chains, see [Knuth Volume 2](https://amzn.to/49zv6Gs) §4.6.3 "Evaluation of Powers."

----

Surprisingly, the "tersest chain" problem is non-trivial in both _Infinite Craft_ and addition-chains.
Knuth writes:

> Several authors have published statements (without proof) that the binary method
> [that is, Russian peasant multiplication] actually gives the minimum possible number of
> multiplications. But that is not true. The smallest counterexample is $$n = 15$$,
> when the binary method needs six multiplications, yet we can calculate $$y = x^3$$
> in two multiplications and $$x^{15} = y^5$$ in three more, achieving the desired result
> with only five multiplications.

This suggests the algorithm Knuth calls the "factor method"; but yet again, you can find numbers
whose optimal chain eludes both the binary method and the factor method! It appears that there
is no fast (non-exponential-time) algorithm that generates an optimal addition chain for _every_ input.

To get an intuitive sense of the difficulty — in particular, why no greedy algorithm helps —
look again at our tersest route to Sandwich:

    Wave = Water + Wind
    Sand = Earth + Wave
    Glass = Fire + Sand
    Wine = Glass + Water
    Sandwich = Sand + Wine

This route to Sandwich passes through Wine on the fourth step.
Now, the tersest route to Wine itself is only three steps:

    Plant = Earth + Water
    Dandelion = Plant + Wind
    Wine = Dandelion + Water

But if you make Wine by that route, you'll never reach Sandwich in the optimum number of steps!

Similarly, our tersest route to 31 was (1 2 3 6 12 14 17 31), passing through 17 on the sixth step.
There are two routes that make 17 in only five steps:

    (1 2 4 8 9 17)
    (1 2 4 8 16 17)

But if you make 17 by either of these routes, you'll never reach 31 in the optimum number of steps!

> Neill Clift of [AdditionChains.com](http://additionchains.com/) produced this example for me —
> my utmost thanks to him! According to Neill, there are exactly five optimal chains for 31 that
> contain the number 17: none of those chains contain (1 2 4 8). Meanwhile there are seventy-two
> other optimal chains for 31 that don't contain 17 at all.

----

UPDATE, April 2024: A third algebraic structure with this shape is
[this one from StackOverflow](https://stackoverflow.com/questions/78228861/choosing-a-sequence-of-bitwise-operations/78229017).
We have one primitive element `A` representing a fair coin flip that lands heads with probability
$$0.5_{10} = 0.1_2$$.
We can construct new elements using either of two binary operations: `&` (representing a coin that
lands heads iff _both_ inputs were heads) and `|` (representing a coin that lands tails
iff _both_ inputs were tails). We're trying to reach a target state representing a coin that
comes up heads with probability $$p$$ (for some $$p = a/2^b$$ between 0 and 1).

For example, we can make a coin that lands heads with probability $$0.5675_{10} = 0.1001_2$$
in either of these ways:

    B = A & A = 0.01          B = A | A = 0.11
    C = B & A = 0.001         R = B & B = 0.1001
    R = C | A = 0.1001

I have not thought much about this particular structure, but it feels just as non-trivial as
addition chains.

----

Still, knowing that _Infinite Craft_ and addition chains are two examples of the same hypergraph
structure doesn't tell me whether there's an accepted name _for_ this particular hypergraph structure.
If you have any leads, please pop over to [MathOverflow](https://mathoverflow.net/questions/466176/what-is-the-proper-name-for-this-tersest-path-problem-in-infinite-craft)
and/or send me an email!

Observe that the addition-chain structure is commutative _and associative_, whereas
the _Infinite Craft_ structure is commutative but non-associative:

    Plant = Water + Earth
    Lava = Earth + Fire
    Smoke = (Water + Earth) + Fire
    Stone = Water + (Earth + Fire)

This makes much of Knuth's discussion (particularly "Graphical representation" and the
generation of equivalent _dual_ addition chains) inapplicable to _Infinite Craft_.

----

To explore the _Infinite Craft_ hypergraph offline — without stressing Neal's backend or needing
to evade his Cloudflare bot-detection filter — you can download a compressed database containing
about 30,000 elements from Tom Fang's GitHub, [szdytom/infinite-craft-dictionary](https://github.com/szdytom/infinite-craft-dictionary/).
Computing the tersest recipe for each element, and inventing a compact way to represent such a
recipe in the database, is left as an exercise for the reader!
