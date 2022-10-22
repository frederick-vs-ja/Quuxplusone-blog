---
layout: post
title: '`has_no_padding_bits`'
date: 2018-06-08 00:02:00 +0000
tags:
  proposal
  rant
excerpt: |
  In my talk ["The Best Type Traits C++ Doesn't Have"](https://youtu.be/MWBfmmg8-Yo?t=41m),
  I present `is_trivially_equality_comparable` (essentially "can I compare these objects
  at runtime with `memcmp`?") and contrast it with P0732's notion-now-perhaps-known-as
  `has_strong_structural_equality` (essentially "can I compare and/or stringify these objects
  at compile time by iterating over their bases and members?").

  In my Aspen presentation, I failed to talk about another motivation for
  `memcmp`-equality: `std::atomic`'s `compare_and_exchange`.
---

{% raw %}
In my talk ["The Best Type Traits C++ Doesn't Have"](https://youtu.be/MWBfmmg8-Yo?t=41m),
I present `is_trivially_equality_comparable` (essentially "can I compare these objects
at runtime with `memcmp`?") and contrast it with P0732's notion-now-perhaps-known-as
`has_strong_structural_equality` (essentially "can I compare and/or stringify these objects
at compile time by iterating over their bases and members?").

In my Aspen presentation, I failed to talk about another motivation for
`memcmp`-equality: `std::atomic`'s `compare_and_exchange`.

    struct S {
        short a;
        short b;
        auto operator<=>(const S&) = default;
    };

    std::atomic<S> a{{1,2}};
    S expected{1,2};
    S desired{3,4};
    a.compare_exchange_strong(expected, desired);

The `atomic` library code
[doesn't actually use the type's comparison operator](https://wandbox.org/permlink/KLnYE47x1eKAfel7)
to do the comparison — instead, it uses `memcmp`.
This means that your `atomic` code is broken, *unless* your type
happens to be trivially comparable.

Specifically, if your struct type has padding bits (like, if you change
`short a` to `char a` in the code sample above), then the `compare_exchange_strong`
will be doing a `memcmp` on the padding bits, which means it might actually fail
when you expected it to succeed, semantically speaking.

(This bug is the subject of active proposal [P0528](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0528r2.html),
which proposes to solve it via built-in magic. I have not closely followed P0528's
progress, but it certainly sounds like I *should* be strongly opposed to its approach.)

I'd like to see the standard library gain a new type trait `has_padding_bits<T>`,
which would immediately see very high adoption:

    struct A {
        short a, b;
    };
    static_assert(!has_padding_bits_v<A>); // OK

    struct B {
        char a;
        short b;
    };
    static_assert(!has_padding_bits_v<A>); // error

The next step would be to require `!has_padding_bits_v<T>` for any class type
used as the template type parameter of `std::atomic::compare_exchange_weak`.
And in fact I'd really like to require something along the lines of

    (
     is_trivially_equality_comparable_v<T> ||
     (!is_equality_comparable_v<T> && !has_padding_bits_v<T>)
    )

Sadly, I predict that if we required `compare_exchange`-able types to actually
*be* `comparable`, it would break an intolerable amount of existing code.
{% endraw %}
