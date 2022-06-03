---
layout: post
title: "Is your constructor an object-factory or a type-conversion?"
date: 2018-06-21 00:01:00 +0000
tags:
  constructors
  initialization
  library-design
  rant
---

I've been meaning to write this one for a while now, but I keep putting it off
because it's not *quite* fully baked. However, if I never write it down, it'll
never get baked. So here it is.

The C++ language takes the very dissimilar notions of _object factories_ and
_type conversions_ and conflates them into a single (and quite revolutionary) notion,
which it calls the _constructor_. Consider the following examples,
for which we will pick on `std::vector` because it's an easy target:

    std::list<char> lst = ...;

    std::vector<char> vec1;
    std::vector<char> vec2(5);
    std::vector<char> vec3(5, 'a');
    std::vector<char> vec4(lst.begin(), lst.end());
    std::vector<char> vec5 { 'a', 'b', 'c' };
    std::vector<char> vec6(lst);
    std::vector<char> vec7(vec1);

(Okay, `vec6` won't compile — you can't convert a `list` to a `vector` like that —
but let's pretend it does, for the sake of this blog post.)

Because of the rules of C++, each of these examples can be rewritten like this:

    auto vec1 = std::vector<char>();
    auto vec2 = std::vector<char>(5);
    auto vec3 = std::vector<char>(5, 'a');
    auto vec4 = std::vector<char>(lst.begin(), lst.end());
    auto vec5 = std::vector<char>{ 'a', 'b', 'c' };
    auto vec6 = std::vector<char>(lst);
    auto vec7 = std::vector<char>(vec1);

And four of them — the three that take only a single argument, plus the one that secretly
takes an `initializer_list` — can be rewritten like this:

    auto vec2 = static_cast<std::vector<char>>(5);
    auto vec5 = static_cast<std::vector<char>>(std::initializer_list<char>{ 'a', 'b', 'c' });
    auto vec6 = static_cast<std::vector<char>>(lst);
    auto vec7 = static_cast<std::vector<char>>(vec1);

This apparent uniformity belies the fact that we have several _very different abstract ideas_
that we're trying to express here!

`vec1` expresses the idea of "object construction": "Just give me a new instance of this type
so that I can mutate its state."
([I'm not a fan of this one.](/blog/2018/05/10/regular-should-not-imply-default-constructible/))

`vec2`, `vec3`, and `vec4` express varying kinds of "object factory": "Take these _inputs_ and apply
some not-necessarily-simple procedure to produce an _output_ of type `vector<int>`."
`vec5` expresses a particularly useful kind of factory, specific to containers, that takes an
explicit _sequence of values_ and builds a container to hold them. (One could argue that
`vec1` expresses the "object factory" idea as well, but my impression is that when we use
the zero-argument constructor, we generally don't care about the value that we get out; we're going
to overwrite or mutate it pretty quickly. So for now I think the zero-argument constructor
is a singular special case.)

Finally, `vec6` expresses a new idea: _type conversion_. We have a value (such as "the sequence
`a, b, c`") represented in one C++ data type, and we're saying, "Take this operand and change
its _type_ without changing its _value_. Give me the same _value_, but represented as a `vector<int>`."
`vec7` expresses the idea of _copying_, which I think can be viewed as a special case of type conversion;
it just happens that the source type and the destination type are exactly the same.
Notice also that `vec5` can be viewed either as a factory ("Take this sequence of inputs and
_put them into_ a container") or as a type conversion ("Take the value represented by this
`initializer_list` and give me the same value but as a container instead.")

I think that a perfect programming language would have different syntax for these different notions.
C++ smushes both notions together into the weird beast it calls a "constructor." Each C++ constructor
is either an _object factory_ (the sadly usual case) or a _type conversion_ (that is, a
"[converting constructor](https://en.cppreference.com/w/cpp/language/converting_constructor)").

When we _use_ a C++ library API, we should strive to make our own code reflect these notions accurately.
Consider these obviously "bad" lines of code:

    auto v = static_cast<std::vector<char>>(5);  // A
    auto p = std::unique_ptr<Widget>(new Widget);  // B
    return std::string();  // C

Line `A` uses "type conversion" notation to invoke a one-argument "object factory" constructor.
Line `B` uses "type conversion" notation when a zero-argument "object factory" function would be more appropriate.
Line `C` uses the singular zero-argument constructor when a one-argument "type conversion" would be more appropriate.
Rewritten as "good" lines of code:

    std::vector<char> v(5);  // A
    auto p = std::make_unique<Widget>();  // B
    return "";  // C

If I had control over the STL's API, I wouldn't allow line `A` to compile. In modern C++, there's
no reason for a function with _factory semantics_ to be expressed as an overload of the constructor.
We should just write a factory function instead!

    auto v = std::vector<char>::with_size(5);

But since I don't control `vector`'s API, I'll settle for making sure my code always uses
the "good" version of line `A`, and never the "bad" version.

We can imagine rewriting _all_ of the problematic constructors above into named factory functions:

    std::vector<char> vec1;
    auto vec2 = std::vector<char>::with_size(5);
    auto vec3 = std::vector<char>::with_repetitions_of(5, 'a');
    auto vec4 = std::vector<char>::from_range(lst.begin(), lst.end());
    auto vec5 = std::vector<char>::from_sequence('a', 'b', 'c');
    auto vec6 = static_cast<std::vector<char>>(lst);
    auto vec7 = static_cast<std::vector<char>>(vec1);

Now each line expresses its intent pretty clearly. Type-conversions are expressed with `static_cast`.
Object factories are expressed with named functions. As a bonus, the factories' names clearly describe
what they do. `vector<int>::with_repetitions_of(1, 2)` can no longer be accidentally confused with
`vector<int>::from_sequence(1, 2)`. `vector<char>::from_sequence("hello", "world")` fails to
compile, and `vector<char>::from_range("hello", "world")` stands out very clearly as a bug.
All's right with the world.


## Why doesn't the STL do this?

Back in C++98, when the STL was designed, you couldn't make a named function that returned an object
without risking expensive copy operations. But in the years since then we've gotten copy elision,
move semantics, and (in C++17) _guaranteed_ copy elision in cases such as the above. So there are
no longer any good reasons for people to be writing factory functions as constructor overloads.

There is still at least one _bad_ reason, though, as explained in my previous posts
["The Superconstructing Super Elider"](/blog/2018/03/29/the-superconstructing-super-elider)
and ["The Superconstructing Super Elider, Round 2"](/blog/2018/05/17/super-elider-round-2).
In C++, constructors are _privileged_ above all other functions, because the C++ standard library
provides a lot of functionality that works with constructors but not with named functions.
Consider:

    vec.emplace_back(x, y, z);

This inserts a new element at the back of `vec`; and it does so by calling one of the element type's
_constructors_. Not a factory function, or a named member function, or any other kind of function:
it specifically wants to call _some overload of the constructor_. So if you write

    vec.emplace_back(Widget::from_inputs(x, y, z));

you'll first create a `Widget` object from the inputs `x, y, z`, and _then_ the library will call
_some constructor_ (even though it's just `Widget`'s move-constructor) to insert that `Widget` into the vector.
["The Superconstructing Super Elider, Round 2"](/blog/2018/05/17/super-elider-round-2) provides
a hacky way to work around this inefficiency. In a perfect language, there would be no inefficiency
to _be_ worked around.

I don't think the C++ standard library will ever do much better; it's pretty fundamentally based
on the constructor as its spiritual center, and I can't imagine that changing.
As they say, ["If I wanted to get there, I wouldn't start from here!"](http://wiki.c2.com/?WouldntStartFromHere)
Besides, C++ puts a high value on backwards compatibility, which means that even if the Committee
suddenly _wanted_ to abandon the "everything is a constructor" model, they wouldn't practically
be able to do so. So the designers of the standard library will continue wrangling for each new
library type — over which constructors should be provided,
which ones should be `explicit` or conditionally `explicit`, which ones should
[take tag parameters](https://akrzemi1.wordpress.com/2016/06/29/competing-constructors/),
whether it's ever okay to `=delete` constructors
(hint: [it's not](https://cplusplus.github.io/LWG/issue2993)),
and so on and so forth.

But if you're writing a _new_ API in C++, one _not_ targeted at getting into the ISO Standard,
I can give you no better advice than:

_Write constructors only for type-conversions. For object-factories, prefer named functions._
