---
layout: post
title: "Ed Catmur's Triliteral esolang"
date: 2024-03-31 00:01:00 +0000
tags:
  esolang
  typography
---

Prolific C++ contributor Ed Catmur died unexpectedly on a [fell run](https://en.wikipedia.org/wiki/Fell_running)
on New Year's Eve 2023. (Many tributes from the C++ and especially the fell-running community; see e.g.
[here](https://slow.org.uk/clubnews/ed-catmur/). He will be missed.)
After Ed's memorial, where a snippet of his CppCon 2023 lightning talk was played, I sifted his GitHub
hoping to find the slides from that talk. (It turns out Phil Nash still had them, and they're now
[publicly available](https://github.com/CppCon/CppCon2023/pull/3).)
Ed left behind a lot of thought-provoking material. One thing that caught my eye — which I had never known
about him — was that he was briefly active on [esolangs.org](https://esolangs.org/wiki/User:Ecatmur), the
wiki for esoteric programming languages.

Programming-related genres of amusement include
"[code golf](https://codegolf.stackexchange.com/)," where you try to write a program
that performs a given task in the fewest bytes of source code
(compare the [demoscene](https://en.wikipedia.org/wiki/Demoscene));
obfuscated code contests such as the [IOCCC](https://www.ioccc.org/2020/carlini/index.html);
composing [quines](https://en.wikipedia.org/wiki/Quine_(computing)) (programs that print their own source code)
and [polyglots](/blog/2020/12/23/a-better-404-polyglot/) (programs that make sense in multiple programming languages
at once); and so on. Another such amusement is the invention of new programming languages that are deliberately
silly or weird or otherwise "esoteric"; these are known as [esolangs](https://esolangs.org/wiki/Esoteric_programming_language).
(The analogous pastime of inventing new non-programming languages, like Esperanto or Sindarin, is called
[conlanging](https://en.wikipedia.org/wiki/Constructed_language).)

An example of an esoteric programming language that intersects with a constructed language is [var'aq](https://esolangs.org/wiki/Var%27aq),
which is supposed to be kind of a [Klingon](https://en.wikipedia.org/wiki/Klingon_language) version
of [Forth](https://en.wikipedia.org/wiki/Forth_(programming_language)).
On the real-human-language side, there's [Qalb](https://en.wikipedia.org/wiki/Qalb_(programming_language))
(قلب), which is kind of an Arabic version of Lisp.

Meanwhile: The Semitic languages (Arabic, Hebrew, etc.) tend to base a lot of their vocabulary on [triconsonantal roots](https://en.wikipedia.org/wiki/Semitic_root).
That's kind of like Indo-European [ablaut](https://en.wikipedia.org/wiki/Indo-European_ablaut) —
a <i>s<b>o</b>ng</i> is something you <i>s<b>i</b>ng</i>, or have <i>s<b>u</b>ng</i>; <i>g<b>ee</b>se</i> is the plural of <i>g<b>oo</b>se</i> —
except much more productive: in Arabic <i>a<b>kt</b>u<b>b</b></i> means "I write," <i>ma<b>kt</b>ū<b>b</b></i> means "written,"
<i><b>k</b>ā<b>t</b>i<b>b</b></i> (plural <i><b>k</b>u<b>tt</b>ā<b>b</b></i>) means "writer,"
<i><b>k</b>i<b>t</b>a<b>b</b></i> (plural <i><b>k</b>u<b>t</b>u<b>b</b></i>) means "book,"
<i>ma<b>kt</b>a<b>b</b></i> means "clerk's office," and so on. Another well-known triliteral root is S-L-M "peace,"
as in <i><b>s</b>a<b>l</b>aa<b>m</b></i>, <i>I<b>sl</b>a<b>m</b></i>, <i>Mu<b>sl</b>i<b>m</b></i>, <i><b>S</b>u<b>l</b>ei<b>m</b>an</i>, etc.

So: Ed developed an esoteric programming language inspired by these triliteral roots.
(Not a _practical_ programming language, mind you, just an _interesting_ one.) As with most esolangs, its grammar is almost entirely nonexistent;
a program in Triliteral is simply a list of words strung together like "...iktb latr lutar iltr sakp usakap...". Each word decodes
into a single command: its triconsonantal root tells us which variable it's operating on, and the "declension" tells us the operation (i.e. the verb).
For example, "iktb" and "iltr" perform the same operation (the `WITH` operation) but on variables K-T-B and L-T-R respectively;
whereas the sequence "sakp usakap" performs two different operations on the same variable (S-K-P). Triliteral's mapping from verbs onto
pronounceable "declensions" is mostly arbitrary. The choice of verbs is pretty standard for an esolang; likewise the general idea
that the program's text gets blatted into memory, one command/word per cell, and so it's easy to write a program that modifies
itself, as in the "Hello World" program below.

Two neat things about Triliteral:

- Numeric literals are encoded (both in the source code and at runtime) via gematria: A=1, B=2, G=3... J=10, Q=100, ..."
    This means that addition and string-concatenation are the same operation: if the sum of the letters in "CRAB" is 211
    and of those in "CAKE" is 34, then the sum of the letters in "CRABCAKE" represents the sum 245.

- The underlying language is abstract; it's merely "encoded" into Latin letters for presentation.
    To encode it into a different script, all you need are a mapping between operations and vowels/declensions,
    and a gematria for encoding integers. Ed provided those pieces for Arabic and Hebrew script as well as for Latin.

Ed's [`helloworld.tlt`](https://github.com/Quuxplusone/triliteral/blob/main/hello%20world.tlt) contains this Latin text:

    aktb ita
    iktb latr lutar
    iltr sakp usakap aklt q iklt asukp asikp
    ilitar
    katab
    asmc a ismc akutab
    usikp
    ob qa qc qc qia lb qit qia qid qc q lg i

which decodes into these Triliteral instructions:

    (QUOT k-t-b) (20)
    (WITH k-t-b) (LOAD l-t-r) (PEEK l-t-r)
    (WITH l-t-r) (LOAD s-k-p) (NOT s-k-p) (QUOT k-l-t) (100) (WITH k-l-t) (MUL s-k-p) (SKIP s-k-p)
    (WCHAR l-t-r)
    (INC k-t-b)
    (QUOT s-m-c) (1) (WITH s-m-c) (POKE k-t-b)
    (JUMP s-k-p)
    (72) (101) (108) (108) (111) (32) (119) (111) (114) (108) (100) (33) (10)

which means:

    Set KTB to 20, the location of the "H" in "Hello world!\n".
    Load LTR from location KTB.
    If LTR is zero, "go to line 100" (that is, quit).
    Write char LTR to the terminal.
    Increment KTB.
    Poke KTB's new value into line 1, replacing the initial "20".
    Go to line 1.
    (here follows the ASCII encoding of "Hello world!\n")

Ed provided several example programs, each recoded into all three scripts. Unfortunately, Ed's recoder program had difficulty with
two corner cases: first whether Latin-script `ts` represents `צ` or `טס`, and second whether Hebrew-script `צ` represents `ts` or `tz`.
The former can be solved by invariably treating `ts` as a digraph, and using `t-s` for `טס`. The latter has no convenient
solution, so it's possible to make a Latin-script program that doesn't round-trip through Hebrew (e.g. if it uses both `ts-t-r`
and `tz-t-r` as variable names). I took the liberty of [forking his GitHub repo](https://github.com/Quuxplusone/triliteral),
implementing the former fix, and renaming some variables to avoid the latter problem, so that all the recoded programs
now work as intended. The Hebrew-script version of the program above is:

```
אכטב יטא
יכטב לאטר לוטאר
ילטר סאכף וסאכאף אכלט ק יכלט אסוכף אסיכף
יליטאר
כאטאב
אסמח א יסמח אכוטאב
וסיכף
עב קא קח קח קיא לב קיט קיא קיד קח ק לג י
```
{:.rtl-code}

and the Arabic-script version is:

```
اكطب يطـا
يكطب لاطر لوطار
يلطر ساكض وساكاض اكلط ق يكلط اسوكض اسيكض
يليطار
كـاطاب
اسمح ا يسمح اكوطاب
وسيكض
عب قا قح قح قيا لب قيط قيا قيد قح ق لج ي
```
{:.rtl-code}

