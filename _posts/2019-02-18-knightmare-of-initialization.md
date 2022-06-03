---
layout: post
title: "The Knightmare of Initialization in C++"
date: 2019-02-18 00:01:00 +0000
tags:
  board-games
  constructors
  initialization
  language-design
  rant
---

[With apologies to Nicolai Josuttis.](https://www.youtube.com/watch?v=7DTlWPgX6zs)

Back in the mid-1990s, when I was about in middle school, I received for Christmas
a copy of Steve Jackson Games' [_Knightmare Chess_](https://amzn.to/2BD4vKw). It's
a deck of cards that you use to augment the regular game of chess. Like, on your turn
instead of moving a piece you might play this card:

> <b>Hidden Passage</b>  
> Move your King to any unoccupied square of the chessboard.

Or this one:

> <b>Fatal Attraction</b>  
> Choose one of your pieces to become a magnet.
> Until it moves or is captured, no piece of any color (except Kings)
> in the eight adjacent squares can move. A piece may pass through
> these squares without effect, but one that stays there (whether or
> not it makes a capture) becomes trapped.

The idea is that the cards increase the "replay value" of chess, and make it more
enjoyable, by injecting a little unpredictability.

Admittedly I have not played _Knightmare Chess_ in decades (if ever), but I find myself
wondering who was its target audience. I mean, who's out there thinking, "I like playing
chess, but I just wish it had a few more _surprise reversals_ from time to time."

----

I feel the same way about C++11's braced initialization syntax — or as it's sometimes
called, "unicorn initialization syntax." This is the C++11-and-later feature that lets us
write things like

    int arr[] {1, 2, 3, 4};
    std::vector<int> vec {1, 2, 3, 4};
    std::pair<int, int> p {1, 2};

but also [things like](https://wandbox.org/permlink/6a8p1C8fun5EOY00)

    int x {42};
    std::vector<char> vc {42, 'x'};
    std::vector<std::string> vs {42, "x"};

That is, you can use `{}` syntax to initialize the elements of a container or algebraic
product type; but you can _also_ use `{}` syntax to call a constructor or even to initialize
a plain old scalar type such as `int`.

So what unicorn initialization gets us is kind of like the _Knightmare Chess_ version of
everyday C++. You can play by the ordinary rules for many turns, and then suddenly you'll
get a surprise sprung on you! For example, did you notice that in the snippet above, `vc`
is constructed with 2 elements but `vs` is constructed with 42 elements? Just as in
_Knightmare Chess_, sometimes the unexpected rule-bending plays to your advantage, and
sometimes it plays to your disadvantage.

> Peter Sarrett of _The Game Report_ called the game "outstanding", remarking that it
> "result[s] [in] an unpredictable game which removes the tedium of standard chess while
> preserving plenty of scope for strategic play." [[Wikipedia](https://en.wikipedia.org/wiki/Knightmare_Chess)]

The visual difference between these two declarations is slight, but the semantic difference
is significant!

    std::vector<char> v{42, 'x'};
    std::vector<char> v(42, 'x');

> Sarrett's only complaints concerned the printing of the cards themselves, as he found
> the wording occasionally confusing and the text "rather small, which makes it difficult
> for players with poorer eyesight to play the game."

One way to improve the readability of your C++ code is to use the simplest, most visually
distinctive way of initializing your variables. For example,

    size_t i{};

should certainly be rewritten as

    size_t i = 0;

And given

    std::vector<char> v{42, 'x'};

I would be at least _tempted_ to rewrite it as

    std::vector<char> v = {42, 'x'};

which communicates the intent a little bit better. (However, notice that the inserted `=` sign
doesn't actually turn off unicorn initialization! `vector<string> vs = {42, "x"}` is [still
perfectly valid C++11](https://wandbox.org/permlink/WW5f8jBUynAXRxX8).)

Several times I've spent extra time studying a piece of code trying to puzzle it out,
simply because on first glance it can be hard to spot a variable declaration when it is mixed
in with other code.

    anonymize_if_needed(get_policies());
    rcode_t rcode = get_rcode();
    define_string("query-type", qtype_.tostring());
    define_string("query-class", qclass_.tostring());
    define_string("result-code", rcode.tostring());
    stringlist policies(get_policies());
    if (policies.size() >= 1) {
        define_string("policy", policies.back());
    }
    policies = get_parent_policies();
    if (policies.size() >= 1) {
        define_string("parent-policy", policies.back());
    }

Now, I am likely preaching to the choir here. And vice versa, this single example isn't
likely to make an instant convert of anyone not already _in_ the choir.
But still: I claim that it would be easier to read this code — easier to suss out what it's
doing at a glance — if the middle line used `=` to highlight the fact that a variable is
being declared and initialized right there.

    stringlist policies = get_policies();

There's simply no reason to use either `()` or `{}` for _most_ variable initializations in C++.
If you're wondering whether to use curly braces or parentheses for construction, your
first question should always be: "Do I really need either one?"

----

Simple guidelines for variable initialization in C++:

- Use `=` whenever you can.

- Use initializer-list syntax `{}` only for element initializers (of containers and aggregates).

- Use function-call syntax `()` to call a constructor, viewed as an [object-factory](/blog/2018/06/21/factory-vs-conversion/).

Thus:

    int i = 0;
    std::vector<int> v = {1, 2, 3, 4};
    Widget w(name, price, quantity);

It would be perfectly reasonable for the programmer to insert an `=` sign
into that last example, too, by switching it over to Almost Always Auto style. Thanks to
[guaranteed copy elision](https://blog.tartanllama.xyz/guaranteed-copy-elision/),
the following line means exactly the same thing as the `=`-less line above:

    auto w = Widget(name, price, quantity);

----

Notice that my guidelines explicitly conflict with
[C++ Core Guideline ES.23](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Res-list),
which dates from back before guaranteed copy elision was a thing, and from not that long
after the invention of unicorn syntax. ES.23 suggests Almost Always Unicorn,
writing the examples above as

    int i {0};
    std::vector<int> v {1, 2, 3, 4};
    Widget w {name, price, quantity};

However, I _really_ hope nobody out there is doing that, especially for `int`s!
The `=` sign is your friend. The `{` symbol is, more often than not, your enemy;
as suggested by the laundry list of exceptions accompanying ES.23.

----

Honestly, chess is challenging enough; I don't think it really needed the addition of
these new cards that sometimes do one thing and sometimes another. Obviously there
_are_ people out there buying this game — I'm sure some of them are even teaching it to
their kids. And I'm sure it can be fun, and I'm sure it leads to some _great_
stories! But, you know, sometimes you just want a simple predictable game of
old-fashioned chess. And then you leave the cards at home.
