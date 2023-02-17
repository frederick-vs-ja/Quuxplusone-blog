---
layout: post
title: "The jargon treadmill"
date: 2023-02-04 00:01:00 +0000
tags:
  c++-style
  etymology
  linguistics
  rant
excerpt: |
  The phrase ["euphemism treadmill"](https://languagehat.com/mcwhorter-on-the-euphemism-treadmill/) refers to the process
  by which ordinary words referring to uncomfortable topics end up becoming "loaded," branded as uncouth
  words _per se_, and thus discarded in favor of new words which haven't yet had the chance to become loaded — which,
  a few years or decades later, are discarded in turn. To take the most obvious example, the Online Etymology Dictionary
  claims that _lavatory_ is attested from 1864; _toilet_ from 1895; _bathroom_ from the 1920s; _restroom_ by the 1930s.
  Of course all these words pre-existed; but they took turns being the mainstream American euphemism for "that room
  with the toilet in it." See Angela Tung's
  ["A Brief History of Lavatory Language"](https://blog.wordnik.com/a-brief-history-of-lavatory-language) (September 2015).

  I've occasionally observed a superficially similar phenomenon that happens in technical fields: an ordinary English word
  will be adopted as technical jargon with a specific technical denotation, which confounds any attempt to use that word
  (in technical writing) with its ordinary colloquial meaning.
---

The phrase ["euphemism treadmill"](https://languagehat.com/mcwhorter-on-the-euphemism-treadmill/) refers to the process
by which ordinary words referring to uncomfortable topics end up becoming "loaded," branded as uncouth
words _per se_, and thus discarded in favor of new words which haven't yet had the chance to become loaded — which,
a few years or decades later, are discarded in turn. To take the most obvious example, the Online Etymology Dictionary
claims that _lavatory_ is attested from 1864; _toilet_ from 1895; _bathroom_ from the 1920s; _restroom_ by the 1930s.
Of course all these words pre-existed; but they took turns being the mainstream American euphemism for "that room
with the toilet in it." See Angela Tung's
["A Brief History of Lavatory Language"](https://blog.wordnik.com/a-brief-history-of-lavatory-language) (September 2015).

I've occasionally observed a superficially similar phenomenon that happens in technical fields: an ordinary English word
will be adopted as technical jargon with a specific technical denotation, which confounds any attempt to use that word
(in technical writing) with its ordinary colloquial meaning. The freshest example is the word "concept." I'd been used
to saying and writing things like

> In C++, initialization and assignment are two completely different concepts

but these days that phrasing seems likely to lead to confusion on the part of some readers — "Wait, those are _concepts?_
What's the definition of assignment look like as a concept?" So since 2017-ish I've made a conscious effort
to switch to unloaded words like "notion" or "idea." For example,
in ["P0732R0 and trivially comparable"](/blog/2018/03/19/p0732r0-and-trivially-comparable/) (2018-03-19)
I talked about "C++'s notion of _trivially destructible_." C++20 does, literally, have
[the concept of `destructible`](https://en.cppreference.com/w/cpp/concepts/destructible), but
_trivial destructibility_ has been downgraded in my book to a mere _notion_.

A much older example — so old that you and I probably didn't live through when it happened — was the loading
of the word "type." I would certainly avoid saying something like

> This routine handles input of two different types...

unless I was talking literally about two different [types](https://en.wikipedia.org/wiki/Data_type). Otherwise I'd try
to say something like "input of two different _kinds_." But then yesterday in the context of Mateusz Pusz's
[`units` library](https://github.com/mpusz/units) I wanted to write something like this:

> We have data types such as `length` and `area`, which represent sort of abstract quantities in our system, and then
> we have other identifiers such as `width` and `altitude`, and `cross_section` and `surface_area`, which represent
> domain-specific strong types that we really don't want to [mix up](https://stackoverflow.com/search?q=mixed-mode%20arithmetic).
> The current naming scheme fails to make it sufficiently clear that `area` and `cross_section`
> (or, `length` and `width`) are completely different <b>kinds</b> of types...

Well, it turns out that [ISO/IEC 80000](https://en.wikipedia.org/wiki/ISO/IEC_80000), the technical standard
in this particular domain, gives a specific technical meaning to the word "kind"!

> <b>kind of quantity</b>  
> aspect common to mutually comparable quantities

Unfortunately, in ISO's terminology, `length` and `width` are two different quantities of _exactly the same_ kind.
So, at least in this particular domain, I must find a new English word that means "type" — er, "kind."
For now I've settled on "sort":

> `area` and `cross_section` are completely different <b>sorts</b> of types...

Are the jargon-inventors going to come next for "notion" and/or "sort", forcing another step on the jargon treadmill?
In practice I wouldn't bet on it — but it does keep me up at night.

----

I suspect that the same phenomenon exists (and even more strongly, that it _existed_) in mathematics, which in the 18th
and 19th centuries consumed English words like "set," "group," and "category" at a rapid clip: you
might struggle to find an unloaded word if you were just trying to talk about an, um, _collection_ of interesting
mathematical objects.

I also suspect that the phenomenon exists to some extent in criminal law. Rather than referring
to someone's behavior being a _nuisance_ (which in most jurisdictions has a specific jargon meaning and thus
might in some contexts be plausibly mistaken for an attempt to allege or imply a crime had been committed),
a lawyer might use a less loaded word like _annoyance_ (which I think hasn't been co-opted as jargon... yet).

Have you experienced this phenomenon in your technical field? I'll add examples here if I hear of any.

----

See also:

* ["WG21: Avoid creating ADL situations on functions with semi-common names"](/blog/2018/11/04/std-hash-value/) (2018-11-04)
    describes the source-code version of this notion, in which "nice" names for free functions (like `size` and `hash_value`)
    must be replaced with progressively uglier names.
