---
layout: post
title: "Hot takes on `flat_map`, `resize_default_init`, and `ranges::to`"
date: 2019-01-28 00:01:00 +0000
tags:
  flat-containers
  kona-2019
  rant
  wg21
---

The pre-Kona WG21 mailing has been out for a little while. Tonight I looked at a few papers;
here's my hot takes.

----

[P0429R6 "A Standard `flat_map`"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0429r6.pdf):
I don't honestly think `flat_map` is well-baked enough to put into _the_ Standard Library; but at least
it's been in Boost for a while (if I understand correctly)
(<b>[EDIT: I do not](/blog/2019/01/29/contra-flat-map)</b>), and it fits into the STL's scheme of
"container adaptors" pretty naturally. The order of the template parameters is awkward but probably
correct.

Some minor typos, such as `Allocator` for `Alloc` in some of the text.

P0429 does innovate in one area: it has a bunch of constructors taking `initializer_list<value_type>&&`.
This [doesn't do what it sounds like](https://wandbox.org/permlink/KZykSIUfi5e2hJkL). Consider:

    FlatSet(std::initializer_list<T>&& il) {
        for (auto&& elt : il) {
            keys_.emplace_back(std::move(elt));
        }
    }

You might think that this "moves-out-of" the elements of the initializer list. That would be
efficient, but also horribly incorrect if it actually happened!

    for (int i=0; i < 10; ++i) {
        FlatSet<std::string> fs { "abc", "def", "ghi" };
        // ...
    }

If "moving-out-of" `initializer_list` elements were permitted, then on the second time through
this loop, all the string elements of the `initializer_list` would be in an already-moved-out-of
state, and bad stuff would happen. (Yes, `initializer_list`s are more or less `static`. The
actual semantics are a huge mess. See Jason Turner's excellent C++Now 2018 talk
["`initializer_list`s Are Broken — Let's Fix Them"](https://www.youtube.com/watch?v=sSlmmZMFsXQ)
for the horrendously gory details.)

Fortunately, `initializer_list` _prevents_ the naïve programmer from moving-out-of its elements,
by [forcing its nested `reference` type to be `const T&`, not `T&`](https://en.cppreference.com/w/cpp/utility/initializer_list).
So in our loop in the constructor above,

    keys_.emplace_back(std::move(elt));

`elt` has type `const std::string&`, which means `std::move(elt)` has type `const std::string&&`,
which means that `emplace_back` will end up perfectly forwarding to `string`'s copy constructor,
_not_ its move constructor. Result: taking an `initializer_list` by non-const rvalue reference
is functionally equivalent to taking `initializer_list` by const lvalue reference, or indeed taking
it by value. ("By value" is the single appropriate way to take `initializer_list`.)

The only thing you accomplish by taking `initializer_list` by rvalue reference is, you break everyone
who thinks they can "perfect-forward" `initializer_list` by value as a special case. For example,
[`std::make_optional`](https://godbolt.org/z/VNvH21).

    auto opt = std::make_optional<FM>(
        // this finds the initializer_list<U> overload of make_optional
        {
            FM::value_type{"1", "abc"},
            FM::value_type{"2", "abc"},
        }
    );

With `FM(initializer_list<value_type>)`, no problem.
But with `FM(initializer_list<value_type>&&)` you get a cryptic error from deep in the
bowels of `make_optional`.

> Always pass `initializer_list` by value.

----

[P1072R3 "`basic_string::resize_default_init`"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1072r3.html):
Ship it! I've watched this one for a while, and I think this is as good as it's going to get.
The library folks have already approved its sister proposal
[P1020](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1020r1.html)
for `make_shared_default_init<int[]>(100)` and `make_unique_default_init<int[]>(100)`. Giving the
same powers to `std::string` just makes sense.

P1072 specifically does _not_ propose to add the `resize_default_init` method to `vector` (or `deque`
or `list` or `forward_list`, all of which have `resize` methods — did you know?), because it's unclear
whether a general facility would be useful, whereas anyone who works with parsing of character data
(e.g. receiving packets from a network socket) knows the use-case for `string::resize_default_init`.

It would be even awesomer to be able to `malloc` a buffer, put some data in it, and then hand that buffer
over to be managed by a `std::string` (or even a `basic_string` with a custom allocator). However,
that rabbit hole is _very_ deep and twisty, and I'm glad the authors of P1072 backed away from it.

But read on!

----

[P1206R1 "`ranges::to`: A function to convert any range to a container"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1206r1.pdf):
`to` is such a great userspace identifier! _Please_ don't drop it into
[the black hole of ADL!](/blog/2018/11/04/std-hash-value/)

The one interesting (and perhaps redeeming?) quality of this proposal is that it is proposing
to give a standard meaning to

    std::string str = "hello world";
    std::vector<char> vec = std::move(str) | ranges::to<std::vector>();

From a human point of view, we can see that a quality implementation should implement that as a
couple of pointer swaps, and ta-da, we have the "awesomer" buffer-ownership-transfer primitive
described above! At least for transferring
ownership between containers that are both provided by the same library (and possibly
it has to be the same library that provides the implementation of `ranges::to`, I'm not sure).

However, Section 10 of P1206R1 implies that this facility will _always_ copy — it is proposed
to take its input range always by const lvalue reference — so, no move semantics for you!
Also, the people who are most interested in speedily converting vectors to strings or vice versa
are probably the same people who are _least_ interested in pulling in all of `<ranges>` (with
its 8-second compile time) just to get the facility.

For people who like their overload resolution faster than their ping time,
note that P1206 proposes to be useable without `operator|` as well:

    std::vector<char> vec = ranges::to<std::vector>(std::move(str));

Also, notice that the `<std::vector>` in the angle brackets is (A) not a typo and (B) nothing
to do with [CTAD](/blog/2018/12/09/wctad/).
The idea is that the library implementor would write _two different overloaded templates_,
both named `to`:

    namespace nonstd {
        template<class Range>
        using range_value_t = iter_value_t<iterator_t<Range>>;
    } // namespace nonstd

    template<class ContainerType, class Range, class... Args>
    ContainerType
    to(Range, Args&&...);

    template<template<class...> class ContainerTemplate, class Range, class... Args>
    ContainerTemplate<nonstd::range_value_t<Range>>
    to(Range, Args&&...);

C++ continues to not-have (and likely will _forever_ not-have) a way to encode "any arbitrary
template with any arbitrary parameters." So if you were hoping for

    auto foo(std::span<int, 5> span)
    {
        return span | ranges::to<std::array>;
    }

then nope, that's still Right Out.

Overall, I think P1206 probably _should_ be accepted as part of Ranges. I like the paper's
"view materialization idiom." I don't like Ranges, or stomping on the name `to`, or clever
metaprogramming shenanigans designed merely to _further_ confuse the CTAD flock; but I think
if we're going to get Ranges then we should also get "view materialization" something like P1206.
