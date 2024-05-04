#!/usr/bin/env python

# Detect English Wikipedia pages whose titles begin with a French personal name
# that lacks hyphens, whereas the corresponding French Wikipedia page begins
# with the same personal name containing hyphens. E.g. hypothetically
# if the English page title is [[Jean Luc Picard (captain)]] but the French title is
# [[Jean-Luc Picard (capitaine)]], we'd like to move the English page to
# [[Jean-Luc Picard (captain)]].
#
# To stderr, as a progress indicator, we print the title of each French page
# with a matching English page, regardless of whether the English title is acceptable.
# To stdout we print the pages that ought to be renamed, with their new name,
# in [[WP:RM]] format.

import requests
import sys

def looks_good(frtitle, name):
  # Don't rename [[Marie Thérèse]] to [[Marie-Thérèse]] or [[Marie Louise]] to [[Marie-Louise]].
  # We also want to respect [[WP:COMMON]], e.g. avoid hyphenating "Marie Antoinette";
  # but that kind of special case can be handled only ad-hoc.
  return (' ' in frtitle)

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
      frtitle = r["title"]
      if looks_good(frtitle, name):
        yield frtitle
    if "continue" not in DATA:
      break
    PARAMS.update(DATA["continue"])

names = [
  'Anne', 'Antoine', 'Auguste', 'Claude', 'François', 'Françoise', 'Guy',
  'Henri', 'Hercule', 'Jacques', 'Jean', 'Louis', 'Louise', 'Luc', 'Marie',
  'Michel', 'Michelle', 'Philippe', 'Pierre', 'Xavier',
]
for name in names:
  for frtitle in frtitles_for_name(name):
    for entitle in entitles_for_frtitle(frtitle):
      assert frtitle.startswith(name + '-')
      print('%s -> %s' % (frtitle, entitle), file=sys.stderr)
      hyphenated_name = frtitle.split()[0].strip(',')
      spaced_name = hyphenated_name.replace('-', ' ')
      if entitle.startswith(spaced_name):
        new_entitle = hyphenated_name + entitle[len(hyphenated_name):]
        assert entitle != new_entitle
        print('* [[:%s]] → {{no redirect|%s}}' % (entitle, new_entitle))
