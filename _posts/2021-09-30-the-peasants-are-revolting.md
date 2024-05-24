---
layout: post
title: "The peasants are revolting"
date: 2021-09-30 00:01:00 +0000
tags:
  jokes
  litclub
  math
---

In my CppCon 2015 talk ["Futures from Scratch,"](https://www.youtube.com/watch?v=jfDRgnxDe7o)
I mentioned in passing that [`std::experimental::when_any`](https://en.cppreference.com/w/cpp/experimental/when_any),
when given an empty list of futures, returns a ready future,
instead of blocking forever (which would be the mathematically
correct result, I said). This morning a YouTube commenter said,
well, it isn't that weird, because the condition "_any_ single future
is ready" is in some sense a "weaker" condition than "_all_
of the futures are ready." This reminded me of a _Wizard of Id_
comic from my youth:

![](/blog/images/2021-09-30-all-of-our-aircraft-is-missing.png){: .meme}

(The reference is to the movie [_One of Our Aircraft is Missing!_](https://en.wikipedia.org/wiki/One_of_Our_Aircraft_Is_Missing) (1942).
According to screenwriter Michael Powell,<sup>[[1]](https://archive.org/details/lifeinmoviesaut00powe/page/387/mode/2up)</sup>
the original working title of that movie was _One of Our Aircraft Failed to Return_,
which is, like, way more of a downer.)

----

The joke here is playing on the fact that if we have $$n$$ aircraft, and also
$$n$$ of our aircraft are missing, then it is logically true that
"all $$n$$ of our aircraft are missing" — even when $$n=0$$. In C++ terms
([Godbolt](https://godbolt.org/z/a5sdfWebE)):

    std::vector<Future> v;
    assert(std::ranges::all_of(v, isReady));
    assert(std::ranges::none_of(v, isReady));
    assert(!std::ranges::any_of(v, isReady));

    auto allAreReady = [](auto... futs) { return (isReady(futs) && ...); };
    auto noneAreReady = [](auto... futs) { return (!isReady(futs) && ...); };
    auto anyIsReady = [](auto... futs) { return (isReady(futs) || ...); };
    assert(allAreReady(/*no arguments*/));
    assert(noneAreReady(/*no arguments*/));
    assert(!anyIsReady(/*no arguments*/));

When $$n=0$$, it is simultaneously true that "all of our futures are ready" and
"none of our futures are ready," _and_ it is obviously false that "any single one
of our futures is ready." (If any one of our futures were ready, then you would be
able to point to which one it is.)

So, I continue to think that `experimental::when_any`'s behavior is not only
mathematically wrong, but surprisingly inconsistent with the rest of the language.

For more on logical quantifiers in C++, see
["Concepts can't do quantifiers"](/blog/2020/08/10/concepts-cant-do-quantifiers/) (2020-08-10)
and ["Covariance and contravariance in C++"](/blog/2019/01/20/covariance-and-contravariance/) (2019-01-20).

----

The above comic was collected in
[ _Wizard of Id #3: The Peasants Are Revolting_](https://archive.org/details/peasantsarerevol0000park/page/n109/mode/1up).
The book cover shows the King, scowling down at the mob: "You can say _that_ again."

In searching for the book (to extract the image above), I came across
[this ads-l thread from 2017](http://listserv.linguistlist.org/pipermail/ads-l/2017-March/147079.html)
which collects a couple other variations on the "revolting" pun-meme, starting with
L. Frank Baum's [_Land of Oz_](https://www.gutenberg.org/files/54/54-h/54-h.htm) (1904).
The Princess-Ida-esque General Jinjur has come to besiege the Emerald City:

> The Guardian of the Gate at once came out and looked at them curiously,
> as if a circus had come to town. He carried a bunch of keys swung round
> his neck by a golden chain; his hands were thrust carelessly into his pockets,
> and he seemed to have no idea at all that the City was threatened by rebels.
> Speaking pleasantly to the girls, he said:
>
> "Good morning, my dears! What can I do for you?"
>
> "Surrender instantly!" answered General Jinjur, standing before him
> and frowning as terribly as her pretty face would allow her to.
>
> "Surrender!" echoed the man, astounded. "Why, it's impossible.
> It's against the law! I never heard of such a thing in my life."
>
> "Still, you must surrender!" exclaimed the General, fiercely. "We are revolting!"
>
> "You don't look it," said the Guardian [...]

Of course _I'd_ run into the pun before as the punch line to Allan Sherman's comic song
["You Went the Wrong Way, Old King Louie"](https://www.youtube.com/watch?v=Ep9fG_ji7T8)
(1963) (the title and tune in this case being a parody of [the 1948 Ray McKinley song](https://www.youtube.com/watch?v=39RQUmiPx7Y)):

> You went the wrong way, old King Louie  
> You made the population cry  
> 'Cause all you did was sit and pet with Marie Antoinette in your place at Versailles [...]
>
> You came the wrong way, old King Louie  
> Now we must put you on the shelf  
> That's why the people are revolting, 'cause Louie, you're pretty revolting yourself!

But I have a new favorite, from the ads-l thread:

> A South American is describing his country to an American woman:
> "...and our most popular sport is bull-fighting."
>
> "Oh!" she exclaims. "Isn't that revolting?"
>
> "No," smiles the South American. "That's our second-most."
