---
layout: post
title: "Two kinds of function template parameters"
date: 2023-12-17 00:01:00 +0000
tags:
  c++-learner-track
  c++-style
  sd-8-adjacent
  templates
excerpt: |
  When you're writing a function template in C++, its template parameters
  will naturally fall into two mutually exclusive categories:

  - Template parameters that the caller _must_ supply in angle brackets;
  - Template parameters that the caller _must not_ supply in angle brackets.
---

When you're writing a function template in C++, its template parameters
will naturally fall into two mutually exclusive categories:

- Template parameters that the caller _must_ supply in angle brackets;
- Template parameters that the caller _must not_ supply in angle brackets.

The core language doesn't distinguish between these two kinds of template parameters;
they go into the same template parameter list. The only real restriction is that
all parameters of the first kind must necessarily precede all parameters of the
second kind.

The poster child for this is `std::make_unique`.

    template<class T, class... Args>
    std::unique_ptr<T> make_unique(Args&&...);

The caller _must_ supply `T` in their angle brackets. The caller _must not_ supply
any of `Args...` in their angle brackets. Again, that "must not" is a _semantic_ rule;
the language doesn't physically enforce it. You physically _can_ call `make_unique` like this:

    auto p = std::make_unique<std::string, int>(3, 'c'); // Wrong

But that's not how `make_unique` is intended to be used. It's supposed to
infer `Args...` from the types of its function arguments.

In fact, the STL reserves the right to break callers who pass template arguments
they're not supposed to. For example, back in C++98
(post-[LWG181](https://cplusplus.github.io/LWG/issue181))
the signature of `make_pair` was:

    template<class T, class U>
    std::pair<T, U> make_pair(T, U);

and so you could physically write:

    int intval = 42;
    auto p1 = std::make_pair<int, std::string>(42, "abc");
    auto p2 = std::make_pair<int, std::string>(intval, "abc");

But in C++11, the signature of `make_pair` changed to use forwarding references
instead of `const&`:

    template<class T, class U>
    std::pair<T, U> make_pair(T&&, U&&);

The initializer of `p1` above continues to compile (because `int&&` can
bind to `42`), but `p2` now fails to compile (because `int&&` won't bind to the lvalue `intval`;
`T` won't deduce as `int&` because we explicitly supplied it as `int` in the angle brackets
in exactly the way I'm telling you not to).

Lots more examples exist; many are so trivial that you wouldn't even recognize them
as examples. E.g.:

    int a[100];
    std::sort(a, a+100); // Right

We _physically could_ write this call as:

    std::sort<int*>(a, a+100); // Wrong

But we shouldn't, because `sort` isn't intended to be called with angle brackets.
By writing the angle brackets, not only are we obfuscating our own code — we're also
risking breakage when we upgrade our vendor's STL.

> I believe we're formally guaranteed that a conforming implementation of C++23
> will support calling `std::sort<int*>` with those template parameters, because
> [[alg.sort]](https://eel.is/c++draft/alg.sort) implies as much. But we're not
> guaranteed that the wording of [alg.sort] won't change!
> A future revision of C++ might change `sort`'s template parameters,
> in the same way C++11 changed `make_pair`'s template parameters.

Another classic example is `swap` (with or without the [`std::swap` two-step](/blog/2020/07/11/the-std-swap-two-step/)):

    Widget a, b;
    Widget wa[10], wb[10];

    using std::swap;

    swap<Widget>(a, b); // Wrong
    swap(a, b); // Right

    swap<Widget>(wa, wb); // Wrong; compiles
    swap<Widget[10]>(wa, wb); // Wrong; doesn't compile
    swap(wa, wb); // Right

    std::string sa, sb;
    swap<std::string>(sa, sb); // Wrong; inefficient
    swap(sa, sb); // Right; efficient

Finally, the most prominent case where you'll be tempted to use the angle brackets,
but I'm telling you it's better not to: `min` and `max`.

    int i = std::min<int>(strlen(s), 42);    // Bad

    int i = std::min(int(strlen(s)), 42);    // Better
    int i = std::min(strlen(s), size_t(42)); // Better
    int i = std::min(strlen(s), 42uz);       // Better (C++23)

## What about "maybe-supply" function template parameters?

A class template like `vector` can be instantiated as either `vector<int>` or `vector<int, Alloc>`.
That allocator parameter sure looks like it doesn't fall into either of my categories — it's
a "maybe-supply" template parameter! But notice that `vector` isn't a function template.
Non-function templates obey a completely different model from function templates:

- Non-function templates don't allow you to pass _only some_ template arguments in their angle brackets;
    e.g. `std::tuple<int, int>{1, 2, 3}` doesn't compile. You can omit a trailing template parameter
    only if the type author has provided a default for that parameter, which indicates they're explicitly
    thinking about that situation.

- Since C++17, class templates can omit the _entire_ angle-bracket part (this is called
    [CTAD](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#ctad) and
    [I recommend never to use it](/blog/2022/10/07/wctad-maybe-unsupported/)).
    But most class templates, e.g. `std::vector`, aren't designed to play well with CTAD;
    they require you to specify at least one argument, and as soon as you write the angle brackets,
    you're physically required to spell out all the (non-defaulted) template arguments.

Still, what if I want to design a function that can take _or not take_ an explicitly supplied
template argument? Something like [`make_optional`](https://en.cppreference.com/w/cpp/utility/optional/make_optional),
which can be called as either:

    auto o1 = make_optional("abc");         // optional<const char*>
    auto o2 = make_optional<string>("abc"); // optional<string>

(This is a bad idea.) Now, what we _want_ to write would simply deduce the function argument's type `A`,
and then we'd default the "maybe-supply" template argument `T` to `A`:

    template<class A, class T = A>
    optional<T> make_optional(A);

But this places `A` in front of `T`, which means the caller must supply `A` in order to
supply `T`! And we can't switch `A` with `T` because then `A` would be used before it was declared.
An ugly, but viable, solution is ([Godbolt](https://godbolt.org/z/TE1T86oYz)):

    template<class T = void, class A, class R = conditional_t<is_void_v<T>, A, T>>
    optional<R> make_optional(A);

A cleaner solution is simply to provide two overloads, as `std::make_optional`
does in real life ([Godbolt](https://godbolt.org/z/GqbT18e38)):

    template<class T, class A>
    optional<T> make_optional(A);
    template<class A>
    optional<A> make_optional(A);

But then you should ask why you're putting these two functions in an overload set in the first place.
(See ["Inheritance is for sharing an interface (and so is overloading)"](/blog/2020/10/09/when-to-derive-and-overload/) (2020-10-09).)
What do you gain by naming them `make_optional(x)` and `make_optional<T>(x)`, as opposed to,
let's say, `make_optional(x)` and `optional<T>(in_place, x)`? I claim: Very little.
(See ["API design advice from Anakin and Obi-Wan"](/blog/2022/12/17/kill-three-member-functions/) (2022-12-17).)
The game isn't worth the candle.

> Overload sets should never take "maybe-supply" template parameters.

## How to enforce a "must-supply" parameter

Very rarely, you might write a template where a "must-supply" parameter is also
deducible from the function argument list.

    template<class T>
    T implicitly_convert_to(T t) {
      return t;
    }

    // Intended usage:
    auto i = implicitly_convert_to<int>(3.14);

    // Preventable mistake:
    auto j = implicitly_convert_to(3.14);

In this case, if we accidentally omitted `<int>`, the call would still compile:
`T` would simply be deduced as `double`, and we'd get unexpected behavior at
compile time. You can prevent this from compiling — and force the caller to
pass `T` explicitly in the angle brackets — by applying a "deduction firewall"
to function parameter `t`. In C++20, that's spelled `type_identity<T>::type`,
or just `type_identity_t<T>` for short.

    template<class T>
    T implicitly_convert_to(std::type_identity_t<T> t) {
      return t;
    }

    auto i = implicitly_convert_to<int>(3.14); // Right, compiles
    auto j = implicitly_convert_to(3.14);      // Wrong, correctly doesn't compile

## How to enforce a "must-not-supply" parameter

More commonly, you'll write a template involving "must-not-supply" parameters,
and (out of sheer paranoia) you'll want to guard against the caller's accidentally
supplying a value for one of them.

    template<class T, class A>
    T implicitly_convert_to(A a) {
      return a;
    }

    // Intended usage: i is 9
    auto i = implicitly_convert_to<int>(9.9999999);

    // Preventable mistake: j is 10
    auto j = implicitly_convert_to<int, float>(9.9999999);

The basic idea is to insert a ["stack canary"](https://en.wikipedia.org/wiki/Buffer_overflow_protection#Canaries)
between the must-supply and must-not-supply parameters, like this:

    template<class T, class Canary = void, class A>
    T implicitly_convert_to(A a) {
      static_assert(is_void_v<Canary>, "Only one explicit template parameter, please");
      return a;
    }

    // No longer compiles
    auto j = implicitly_convert_to<int, float>(9.9999999);

    // But a truly hostile caller can still do it: k is 10
    auto k = implicitly_convert_to<int, void, float>(9.9999999);

An anonymous reader points out that you can make this truly foolproof — and eliminate
the `<type_traits>` dependency at the same time — by making the canary a _pack_.
At least in present-day C++, the first pack inexorably expands to consume all the remaining
explicit template arguments, so our hostile caller _can't_ bypass it.

> So instead of a "canary," it's more of a "pelican." A pack also acts as an explicit-argument-specification firewall;
> but I've called one thing a "firewall" in this blog post already. To stretch a plumbing
> metaphor, we might call it a "[surge tank](https://en.wikipedia.org/wiki/Surge_tank)":
> it goes unused in normal operation, but when too many arguments come in at once,
> it can safely hold the excess so as not to damage the infrastructure downstream of it.

    template<class T, class... Canary, class A>
    T implicitly_convert_to(A a) {
      static_assert(sizeof...(Canary) == 0, "Only one explicit template parameter, please");
      return a;
    }

    // No longer compile
    auto j = implicitly_convert_to<int, float>(9.9999999);
    auto k = implicitly_convert_to<int, void, float>(9.9999999);

## A third kind of template parameter

Pre-C++20, we had a third kind of template parameter too; the kind that
you _must not_ supply in the angle brackets, but which wouldn't be used
in the template's body either. This kind of parameter encodes what we now call
"constraints" on the template.

    template<class T, class A,
             class E = enable_if_t<is_convertible_v<A, T>>>
    T implicitly_convert_to(A arg) { return T(arg); }

Here `T` is "must-supply," `A` is "must-not-supply," and `E` is a
"constraint." `convert` won't participate in overload resolution
unless `is_convertible_v<T, A>` is true... or, unless the caller
breaks our rule and supplies `E` in the angle brackets!

    int i = 42;
    auto j = implicitly_convert_to<intptr_t>(&i); // Good; doesn't compile
    auto k = implicitly_convert_to<intptr_t, int*, void>(&i); // Bad; compiles!

Pre-C++20, you can make it foolproof by using a non-type template parameter:

    template<class T, class A,
             enable_if_t<is_convertible_v<A, T>, int> E = 0>
    T implicitly_convert_to(A arg) { return T(arg); }

In C++20, this kind of template parameter should eventually go extinct,
because we can always replace it with a `requires` clause instead.
See ["Prefer core-language features over library facilities"](/blog/2022/10/16/prefer-core-over-library/) (2022-10-16).

    template<class T, class A>
      requires is_convertible_v<A, T>
    T implicitly_convert_to(A arg) { return T(arg); }
