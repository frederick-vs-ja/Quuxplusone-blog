---
layout: post
title: "Counting distinct n-state DFAs"
date: 2024-07-27 00:01:00 +0000
tags:
  math
  pretty-pictures
excerpt: |
  On the occasion of [the confirmation of BB(5)](https://scottaaronson.blog/?p=8088)
  (a problem involving the enumeration of Turing machines),
  Thomas Baruchel asks the following simpler question about finite state machines:

  > Given an alphabet of size $$L$$, how many distinct languages over that alphabet
  > are recognizable by a [complete DFA](https://en.wikipedia.org/wiki/Deterministic_finite_automaton#Complete_and_incomplete) with $$n$$ states?
---

On the occasion of [the confirmation of BB(5)](https://scottaaronson.blog/?p=8088)
(a problem involving the enumeration of Turing machines),
Thomas Baruchel asks the following simpler question about finite state machines:

> Given an alphabet of size $$L$$, how many distinct languages over that alphabet
> are recognizable by a [complete DFA](https://en.wikipedia.org/wiki/Deterministic_finite_automaton#Complete_and_incomplete) with $$n$$ states?

For $$L=1$$, this is the partial sums of OEIS [A059412](https://oeis.org/A059412):
A059412($$n$$) counts the number of distinct unary languages recognizable by a *minimal* complete
DFA with $$n$$ states, where a DFA is defined to be "minimal" if and only if no smaller DFA
recognizes the same language.

For $$L=2$$, this is the partial sums of [A129622](https://oeis.org/A129622), which
counts the number of distinct binary languages recognizable by a minimal complete
DFA with $$n$$ states. (However, as of this writing, A129622(1)=2 is omitted from OEIS,
which means you'll have to remember to add it to those partial sums yourself.)

For $$L=3$$, the count of distinct languages over the alphabet {`a`, `b`, `c`}
is not yet in the OEIS, but its analogous sequence would begin $$2, 112, 41928,\ldots$$.

It seems more natural to me to count the *total* number of languages recognizable by
$$n$$-state DFAs; you can simply subtract adjacent elements to obtain the OEIS sequence.
The count will always be an even number, because for any DFA that accepts a language *including*
the empty string, you can turn it into a DFA that accepts the exactly complementary language
(*not including* the empty string) by simply flipping all the states' "accept" bits.

In the following table, column `L=1` is the partial sums of [A059412](https://oeis.org/A059412).
Column `L=2` is the partial sums of [A129622](https://oeis.org/A129622).
Row `n=2` seems to be $$2(2^{2L} - 2^L + 1)$$ (i.e., 2×[A020515](https://oeis.org/A020515), or 1+[A092440](https://oeis.org/A092440)).

    Count of distinct languages over an alphabet of size L
    which are recognizable by a DFA with at most n states.

          L=1   L=2      L=3     L=4     L=5      L=6   L=7    L=8
    n=1   2     2        2       2       2        2     2      2
    n=2   6     26       114     482     1986     8066  32514  130562
    n=3   18    1054     42042   1353214 39676458
    n=4   48    57068            2048646
    n=5   126   3762374
    n=6   306   290480170
    n=7   738   25784367022
    n=8   1716

The most impressive of these numbers are just copied from OEIS.
Simple Python and C++ code for the smaller numbers is [here](https://github.com/Quuxplusone/RecreationalMath/tree/master/CountDFAs).

The six distinct languages over alphabet {`a`} recognizable by 2-state DFAs are these three and their complements:

|-----|-----|
| ![](/blog/images/2024-07-27-dfa-L1-n1a.svg) | `.*` |
| ![](/blog/images/2024-07-27-dfa-L1-n2b.svg) | `.*.` |
| ![](/blog/images/2024-07-27-dfa-L1-n2c.svg) | `(..)*` |
{:.dfa-table}

The twenty-six distinct languages over alphabet {`a`, `b`} recognizable by 2-state DFAs are these 13 and their complements:

|-----|-----|-----|-----|
| ![](/blog/images/2024-07-27-dfa-L2-n1a.svg) | `.*` | | |
| ![](/blog/images/2024-07-27-dfa-L2-n2b.svg) | `..*` | | |
| ![](/blog/images/2024-07-27-dfa-L2-n2c.svg) | `(..)*` | | |
| ![](/blog/images/2024-07-27-dfa-L2-n2d.svg) | `a*` | ![](/blog/images/2024-07-27-dfa-L2-n2e.svg) | `b*` |
| ![](/blog/images/2024-07-27-dfa-L2-n2f.svg) | `.*a` | ![](/blog/images/2024-07-27-dfa-L2-n2g.svg) | `.*b` |
| ![](/blog/images/2024-07-27-dfa-L2-n2h.svg) | `(a|b.)*` | ![](/blog/images/2024-07-27-dfa-L2-n2i.svg) | `(b|a.)*` |
| ![](/blog/images/2024-07-27-dfa-L2-n2j.svg) | `(.a*b)*` | ![](/blog/images/2024-07-27-dfa-L2-n2k.svg) | `(.b*a)*` |
| ![](/blog/images/2024-07-27-dfa-L2-n2l.svg) | `(a|ba*b)*` | ![](/blog/images/2024-07-27-dfa-L2-n2m.svg) | `(b|ab*a)*` |
{:.dfa-table}

## Equivalence "up to recoloring of the alphabet"

Notice that the above table has only 8 languages and their complements "up to recoloring of the alphabet";
for example, to recognize the language `b*` it suffices to map all `b`s in your input to `a`s and vice versa,
and then feed the resulting string to the DFA for `a*`.

> To recognize `.*` it suffices to map both letters to `a` and then use the DFA for `a*`; but the reverse isn't
> true — you can't use the DFA for `.*` to recognize the language `a*` — and so we don't consider those two
> languages equivalent.

As the alphabet size $$L$$ increases, the count of languages "equivalent up to recoloring of the alphabet"
also increases. Maybe we should count each equivalence class only once. This makes the count of distinct
$$n$$-state DFAs converge to a well-defined sequence $$a(k) = f(n=k, L\geq k^2) = 2, 26,\ldots$$.

    Count of distinct languages over an alphabet of size L
    which are recognizable by a DFA with at most n states,
    up to recoloring of the alphabet.

          L=1   L=2      L=3     L=4     L=5
    n=1   2     ...
    n=2   6     16       24      26      ...
    n=3   18    540      7038
    n=4   48    28640
    n=5   126   1882192
    n=6   306
    n=7   738

The twenty-six distinct-up-to-recoloring languages recognizable by *any* 2-state DFA
are these 13 and their complements. The first eight rows of this table match the eight rows of the
previous table; the next five rows are new. This set of twenty-six DFAs is closed — if you add
to any one of these diagrams the state transitions for a new letter `e`, you'll find that you've
merely duplicated one of the other diagrams in the set.

|-----|-----|
| ![](/blog/images/2024-07-27-dfa-L2-n1a.svg) | `a*` |
| ![](/blog/images/2024-07-27-dfa-L2-n2b.svg) | `aa*` |
| ![](/blog/images/2024-07-27-dfa-L2-n2c.svg) | `(aa)*` |
| ![](/blog/images/2024-07-27-dfa-L2-n2d.svg) | `a*` |
| ![](/blog/images/2024-07-27-dfa-L2-n2f.svg) | `(a|b)*a` |
| ![](/blog/images/2024-07-27-dfa-L2-n2h.svg) | `(a|b(a|b))*` |
| ![](/blog/images/2024-07-27-dfa-L2-n2j.svg) | `((a|b)a*b)*` |
| ![](/blog/images/2024-07-27-dfa-L2-n2l.svg) | `(a|ba*b)*` |
| ![](/blog/images/2024-07-27-dfa-L4-n2a.svg) | `(b|c|(a(a|c)*b))*` |
| ![](/blog/images/2024-07-27-dfa-L4-n2b.svg) | `(b|c|(ac*(a|b)))*` |
| ![](/blog/images/2024-07-27-dfa-L4-n2c.svg) | `(c|((a|b)(b|c)*a))*` |
| ![](/blog/images/2024-07-27-dfa-L4-n2d.svg) | `(c|((a|b)b*(a|c)))*` |
| ![](/blog/images/2024-07-27-dfa-L4-n2e.svg) | `(c|d|((a|b)(b|d)*(a|c)))*` |
{:.dfa-table}
