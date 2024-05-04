---
layout: post
title: "Using Wikipedia's API to find inconsistently hyphenated French names"
date: 2024-05-04 00:01:00 +0000
tags:
  sre
  web
---

The other day I noticed that a lot of English Wikipedia articles about French people
for some reason use a space between parts of the [given name](https://en.wikipedia.org/wiki/French_name#Given_names)
where primary sources and/or the French Wikipedia use a hyphen; for example,
the English Wikipedia (as of May 2024) has "[Marie Thérèse Geoffrin](https://en.wikipedia.org/w/index.php?title=Marie_Th%C3%A9r%C3%A8se_Geoffrin&redirect=no)"
where both [the _Bibliothèque nationale_](https://catalogue.bnf.fr/ark:/12148/cb12517719j) and
[_Encyclopaedia Britannica_](https://www.britannica.com/biography/Marie-Therese-Rodet-Geoffrin)
have "[Marie-Thérèse Geoffrin](https://catalogue.bnf.fr/ark:/12148/cb12517719j)."

I wrote a Python script that uses the Mediawiki API to enumerate the articles with this inconsistency,
and generate [requested moves](https://en.wikipedia.org/wiki/Wikipedia:Requested_moves) for each page
where the French article's title begins with e.g. "Marie-Thérèse" and the English article's title begins
with "Marie Thérèse." Wikipedia's API documentation is a little scattered, but really great, as documentation
goes. It includes lots of runnable examples. My script ended up using:

* [Wikipedia's `prefixsearch` API](https://www.mediawiki.org/wiki/API:Prefixsearch)
* [Wikidata's `wbgetentities` API](https://www.mediawiki.org/wiki/Wikibase/API), thanks to [this StackOverflow answer](https://stackoverflow.com/a/77748707/1424877)
* [the common pagination API](https://www.mediawiki.org/wiki/API:Continue)

The first of these lets us query for all French Wikipedia articles whose titles begin with e.g. `"Jean-"`:

    def frtitles_for_name(name):
      S = requests.Session()
      PARAMS = {
        "action": "query",
        "format": "json",
        "list": "allpages",
        "aplimit": "500",
        "apprefix": (name + '-'),
        "apfilterredir": "nonredirects",
      }
      while True:
        R = S.get(url="https://fr.wikipedia.org/w/api.php", params=PARAMS)
        DATA = R.json()
        for r in DATA["query"]["allpages"]:
          yield r["title"]
        if "continue" not in DATA:
          break
        PARAMS.update(DATA["continue"])

The second lets us find the corresponding English Wikipedia article and check whether its
title begins with `"Jean-"` or with `"Jean "`:

    def entitles_for_frtitle(frtitle):
      S = requests.Session()
      R = S.get(url="https://www.wikidata.org/w/api.php", params={
        "action": "wbgetentities",
        "sites": "frwiki",
        "titles": frtitle,
        "props": "sitelinks",
        "format": "json",
      })
      DATA = R.json()
      for r in DATA["entities"].values():
        entitle = r.get("sitelinks", {}).get("enwiki", {}).get("title", None)
        if entitle is not None:
          yield entitle

I used `yield` to make both functions into generators, even though we expect `entitles_for_frtitle`
to return only a single result (or zero results), simply because that lets me write a nice clean
main function like this:

    for name in ['Anne', 'Claude', 'Guy', 'Jean']:
      for frtitle in frtitles_for_name(name):
        for entitle in entitles_for_frtitle(frtitle):
          hyphenated_name = frtitle.split()[0].strip(',')
          spaced_name = hyphenated_name.replace('-', ' ')
          if entitle.startswith(spaced_name):
            new_entitle = hyphenated_name + entitle[len(hyphenated_name):]
            print('* [[:%s]] → {{no redirect|%s}}' % (entitle, new_entitle))

This does miss a few cases, e.g. when the English title is missing not only the hyphen but
also an accent (e.g. "Henri Evrard" for "Henri-Évrard"). And it has false positives for cases
that aren't personal names, for example the name of the 1956 film
[_Marie Antoinette Queen of France_](https://en.wikipedia.org/wiki/Marie_Antoinette_Queen_of_France)
(French: [_Marie-Antoinette reine de France_](https://fr.wikipedia.org/wiki/Marie-Antoinette_reine_de_France)).
But it's pretty darn good as a first pass.

See the complete Python script [here](/blog/code/2024-05-04-french-names.py), and look for those
Wikipedia pages to get moved at some point in the near future.
