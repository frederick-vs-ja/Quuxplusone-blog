---
layout: post
title: 'TIL: `nullopt_t` is not equality-comparable, but `monostate` is'
date: 2022-03-07 00:01:00 +0000
tags:
  ranges
  cpplang-slack
  standard-library-trivia
  today-i-learned
  value-semantics
---

On [Slack](https://cppalliance.org/slack/),
Kilian Henneberger asked for some STL types that are copyable but not
equality-comparable. One example is `std::function<int()>`; see
["On `function_ref` and `string_view`"](/blog/2019/05/10/function-ref-vs-string-view/) (2019-05-10).
The simplest example is `struct S {};` — for compatibility with C, C++98 provided
every class type with a copy constructor, but none of them with comparison operators.

So, I said, a good STL example would be [`std::monostate`](https://en.cppreference.com/w/cpp/utility/variant/monostate)
from the `<variant>` header, which is basically just a trivial tag type, right? _Wrong!_

    constexpr std::monostate m;
    static_assert(m == m);

It turns out that `monostate` needs to be totally ordered, because if it weren't, then
`variant<int, monostate>` wouldn't be totally ordered either.

But you know what's like a `variant<int, monostate>`? `optional<int>`!
Everyone knows that `optional<int>` is totally ordered, right? So:

    constexpr std::nullopt_t n;
    bool b = (n == n);  // Error: does not compile!

It turns out that `std::nullopt_t` is _not_ equality-comparable.

This makes sense in hindsight, actually. My new mental model is:

- `std::nullopt_t` is a tag type, like [`std::in_place_t`](https://en.cppreference.com/w/cpp/utility/in_place)
    or even `std::nullptr_t`. Its only purpose is to be used in _syntactic_ constructs
    like `myOptional = std::nullopt`. It's not a "value-semantic" type; its only job
    is to participate in overload sets and then get out of the way as fast as possible.
    An `optional` never really "holds" a `nullopt_t` value. In a value-semantic,
    Tony-van-Eerd kind of sense, `nullopt_t` doesn't _have_ any "values."
    You _physically can_ make a `vector<nullopt_t>`, just like you can make a `vector<nullptr_t>`
    or a `vector<in_place_t>`, but you _shouldn't_.
    Notably, you [physically cannot](https://eel.is/c++draft/optional#optional.general-3)
    make an `optional<nullopt_t>`.

- `std::monostate` is a value-semantic type, like `bool` — it just has one fewer value
    in its domain. It can be stored in variants, or containers, or `set`s (it's ordered!),
    or `unordered_set`s (it's hashable!), or anywhere else you might use a value-semantic
    type like `bool`. It's totally fine to make an `optional<monostate>`.


## Given time, Ranges will erase this mental model

Problem: The C++20 Ranges library has Opinions about comparability. Ranges deals _only_ with
value-semantic (or reference-semantic) types; it has very little respect for syntactic fillips
like `nullopt` or `nullptr`. For example, Ranges assumes that "Type X is comparable with type Y"
must necessarily mean "_Values_ of type X are comparable with _values_ of type Y," i.e., X and Y
cover the same _domain_, i.e., `common_reference_t<X, Y>` must exist and be equality-comparable with
itself.

    template <class T, class U>
    concept equality_comparable_with =
        equality_comparable<T> &&
        equality_comparable<U> &&
        common_reference_with<
            const remove_reference_t<T>&,
            const remove_reference_t<U>&> && ~~~;

This breaks badly for syntactic tags like `nullptr_t` and `nullopt_t`. And because Ranges
concepts are used to constrain all the Ranges algorithms, we end up with ridicule-worthy
situations like ([Godbolt](https://godbolt.org/z/qEMenWqEK)):

    std::unique_ptr<int> a[10];
    std::optional<int> b[10];

    auto it = std::find(a, a+10, nullptr);      // OK
    auto jt = std::find(b, b+10, std::nullopt); // OK

    std::ranges::find(a, a+10, nullptr);      // Error: no viable overload
    std::ranges::find(a, a+10, std::nullopt); // Error: no viable overload

(By the way, if you're not used to Concepts error messages, take a look at the error
messages in that Godbolt!) The problem is that, because `nullopt_t` is not
`equality_comparable` with itself (or in the other case because
`const unique_ptr<int>&` is not convertible to `common_reference_with<unique_ptr<int>, nullptr_t>`),
Ranges thinks that the two types are not comparable at all.

This is the subject of Justin Bassett's recent paper
[P2405 "`nullopt_t` and `nullptr_t` should both have `operator<=>` and `operator==`"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2405r0.pdf)
(July 2021), currently slated for ["not C++23"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2489r0.html#not-for-cpp23)
but maybe C++26.

In other words, the gravitational pull of Ranges will probably end up eroding
the distinction I just described between "value-semantic" types like `monostate` and
"syntactic tag" types like `in_place_t` and `nullptr_t`, simply because Ranges cannot
(currently) deal with types that aren't value-semantic in the most heavyweight sense possible.
Maybe this is a good thing — maybe "syntactic tag" types are an abomination and we should
be glad if they all turn into proper values. But I doubt it.
