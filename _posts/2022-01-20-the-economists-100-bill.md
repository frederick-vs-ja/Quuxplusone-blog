---
layout: post
title: "The economist's $100 bill, and the virtue of consistency"
date: 2022-01-20 00:01:00 +0000
tags:
  jokes
  slogans
  sre
excerpt: |
  > Two economists are walking down the street. One of them notices
  > what appears to be a $100 bill just lying on the sidewalk, and
  > points it out to the other. "Nonsense," declares his friend.
  > "If that were a real $100 bill, someone would have picked it
  > up already."
---

{% raw %}
> Two economists are walking down the street. One of them notices
> what appears to be a $100 bill just lying on the sidewalk, and
> points it out to the other. "Nonsense," declares his friend.
> "If that were a real $100 bill, someone would have picked it
> up already."

It's a joke, but it has a widely applicable point: When you see
something that *looks* like a simple solution that could have
been done anytime (such as tearing down
[Chesterton's Fence](https://www.chesterton.org/taking-a-fence-down/)),
you should be reflexively skeptical. If it were really that easy,
wouldn't someone have done it already? There must be a reason it
hasn't already been done!

(See also: "Why isn't every psychic a millionaire?")

Of course this assumes that we're working on a group project,
where important historical rationales are frequently undocumented.
On a _solo_ project, this mantra doesn't apply at all — if you're
Will Smith in [_I Am Legend_](https://en.wikipedia.org/wiki/I_Am_Legend_(film))
and you see a $100 bill on the sidewalk, you probably _should_ believe
your eyes. (On the other hand, if you're Will Smith in _I Am Legend_,
what would you need $100 for?)

A corollary to this principle — or at least I think of it as a corollary —
is, "If this works, why isn't everyone doing it?" This is especially applicable
in code reviews. It explains why consistency is a virtue: _inconsistency_
between A and B almost always means that one of them has fixed a bug that
still exists in the other (or vice versa, one has introduced a bug that
the other was intentionally avoiding).

For example, suppose we have just noticed that a GitHub Actions automation
is breaking because it can't deal with spaces in the user's first name. We've got
a YAML file somewhere in our codebase that looks like

    - name: Hypothetical example
      run: |
        ./hypothetical.py \
          --name ${{ event.user.firstname }} \
          --surname ${{ event.user.lastname }}

and it all works fine for our first hundred users like `Tom` and `Mary-Sue`,
but then we get a user named `Billy Bob` and our job starts throwing errors
like

    > Hypothetical example
        ./hypothetical.py \
          --name Billy Bob \
          --surname Thornton
      shell: /usr/bin/bash -e {0}
    usage: hypothetical.py [-h] --name FIRST --surname LAST
    hypothetical.py: error: unrecognized arguments: Bob

Someone suggests a fix:

     - name: Hypothetical example
       run: |
         ./hypothetical.py \
    -      --name ${{ event.user.firstname }} \
    +      --name '${{ event.user.firstname }}' \
           --surname ${{ event.user.lastname }}

This patch immediately has a code smell, because it applies the (hypothetical) fix
only to the `--name` argument, instead of consistently to all arguments. Even if
this fixed the bug for `Billy Bob`, we would just hit the exact same bug later on
for a surname like `Lo Truglio`. When I see a patch like this, I think:

> If this really works, <b>why isn't everyone doing it?</b>

The patch should apply its fix to _both_ arguments — and to all other arguments
in this YAML file, and to all other YAML files in the repository — or maybe it's
just not the $100 bill it looks like.

----

In this particular case, the patch is probably wrong anyway, because it can't
deal with first names like `T'Challa` or last names like... well, one will come
to me eventually. Remember [Little Bobby Tables](https://xkcd.com/327/) —
and remember that roll-your-own shell escaping works about as well as
roll-your-own crypto.

But what _is_ GitHub Actions' shell-escaping story? As of this writing,
[nobody seems to know.](https://stackoverflow.com/questions/65193383/how-to-set-path-that-contains-spaces-in-a-yaml-file-in-a-github-workflow)
{% endraw %}
