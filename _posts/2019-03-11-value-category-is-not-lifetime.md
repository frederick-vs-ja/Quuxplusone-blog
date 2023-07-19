---
layout: post
title: "Value category is not lifetime"
date: 2019-03-11 00:01:00 +0000
tags:
  c++-learner-track
  library-design
  metaprogramming
  parameter-only-types
  pitfalls
  slogans
excerpt: |
  Here's another slogan that needs more currency:

  > Value category is not lifetime.
---

Here's another slogan that needs more currency:

> Value category is not lifetime.

What I mean by this mantra is probably best shown by example.


## Two examples of a common C++ problem

Suppose we design a `String` class that looks like this:

    class String {
        std::string s_;
    public:
        explicit String(const char *s) : s_(s) {}
        operator string_view() const { return s_; }
    };

And suppose we design a `TokenIterator` class that looks like this:

    class TokenIterator {
        const char *s_;
    public:
        explicit TokenIterator(const std::string& s) : s_(s.data()) {}
        auto& operator++() {
            // complicated logic to advance `s_` through the string
        }
    };

These are extremely idiomatic C++ classes. But they each suffer from a minor flaw
that might gnaw at the writer's conscience: they're both prone to _dangling references_.
Consider these two ways of using `String`'s conversion to `string_view`:

    String s("hi");
    std::string_view sv1 = s;  // OK

    std::string_view sv2 = String("hi");  // BAD!

The "BAD" snippet creates a `string_view` that refers to the temporary `String("hi")` and
then immediately destroys that temporary, meaning that any further use of `sv2` will
dereference a dangling pointer and cause undefined behavior.

> For more on `string_view` in particular, and why I don't see this as a difficult problem,
> see my blog post ["`string_view` is a parameter-only type"](/blog/2018/03/27/string-view-is-a-borrow-type/)
> (2018-03-27).

Or consider these two ways of using `TokenIterator`:

    std::string s("hello world");
    auto tk1 = TokenIterator(s);  // OK

    auto tk2 = TokenIterator("hello world");  // BAD!

The "BAD" snippet creates a temporary `std::string` from `"hello world"`; then creates a `TokenIterator`
referring to that temporary; and then destroys the temporary `std::string`, meaning that any further use
of `tk2` will dereference a dangling pointer and cause undefined behavior.


## The unfortunate "solution"

Given this problem, some prominent C++ libraries have taken to solving it by
the following faulty syllogism:

- For safety, we want to avoid taking references to objects whose lifetime is shorter than the lifetime of the reference itself.

- Therefore we should avoid taking references to objects with _temporary_ lifetime.

- Temporary objects are generally _rvalues_.

- Therefore we should reject taking references to things whose value category is _rvalue_.

This chain of illogic tempts library writers to patch `String` and `TokenIterator` like this:

    class String {
        std::string s_;
    public:
        explicit String(const char *s) : s_(s) {}
        operator string_view() const & { return s_; }
        operator string_view() const && = delete;
    };

    class TokenIterator {
        explicit TokenIterator(const std::string& s) : s_(s.data()) {}
        explicit TokenIterator(const std::string&&) = delete;
    };

[We check our examples from earlier](https://godbolt.org/z/fq4YIx)
and find that they seem to be doing exactly what we want!
Our cases that had led to dangling references now get caught at compile time.

    String s("hi");
    std::string_view sv1 = s;  // still OK

    std::string_view sv2 = String("hi");  // Compiler error!

So it seems that we've found a neat heuristic to distinguish babies from bathwater.
Several prominent C++ libraries — including C++14's
[`regex_iterator`](https://en.cppreference.com/w/cpp/regex/regex_iterator/regex_iterator) —
shipped with heuristics like this, and the technique has been mentioned in places such as
["Guidelines For Rvalue References In APIs"](https://foonathan.net/blog/2018/03/26/rvalue-references-api-guidelines.html)
(Jonathan Müller, March 2018).

However...

> Value category is not lifetime.


## Babies that look like bathwater

Suppose we code up our "dangling-reference-proof" `String` class. And then after it's
shipped, we get a bug report from a user. The following code
[doesn't compile anymore](https://godbolt.org/z/-pwzef):

    void print_it(std::string_view param) {
        std::cout << param << std::endl;
    }

    void test() {
        String hello("hello");
        String world("world");
        print_it(hello + world);
    }

See, here the expression `hello + world` yields a temporary `String` object, and we're trying to
pass that `String` to a function that takes a `string_view` parameter. There's nothing intrinsically
wrong with this code. `string_view` even seems like the *most correct* parameter type for `print_it` —
given that `print_it` is not going to modify its parameter, and doesn't care what its actual type is,
as long as it's string-ish.

However, the author of `String` has `=delete`d the `operator string_view()` that would have
converted this temporary `String` into a `string_view`. And so we can't use temporary `String`s
with `print_it` anymore.

What went wrong? Well, we successfully disabled conversion-to-`string_view` for (some) *temporary objects*.
But just because an object is *temporary*, doesn't mean that its lifetime is necessarily shorter than
the lifetime of the reference we'd take! In our `print_it` example, the lifetime of the temporary `String`
is strictly longer than the lifetime of `string_view param`. So initializing `param` from a temporary
isn't a source of dangling references in this case.

Our heuristic used _value category_ (rvalue-ness) as a proxy for _lifetime_ (likelihood of dangling).
But value category is not lifetime!


## Bathwater that looks like babies

We code up our "dangling-reference-proof" `TokenIterator` class, and trumpet its new easy-to-use,
bug-proof interface on our company blog. Then, we get a bug report from a user. The following code
[compiles and runs for them...](https://godbolt.org/z/xgMjFu) and crashes, due to a dangling reference!

    TokenIterator make_iterator(const std::string& haystack) {
        return TokenIterator(haystack);
    }

    void test() {
        auto tk = make_iterator("hello world");
        auto first_token = *tk;  // compiles and CRASH!
    }

What went wrong? Well, we successfully disabled construction from rvalue `string`s.
But just because an object is *not an rvalue*, doesn't mean that it's not a *temporary!*
In our `make_iterator` example, `haystack` is an lvalue, but the object to which it refers
is still a temporary that will be destroyed before the end of `tk`'s lifetime.

Our heuristic used _value category_ (lvalue-ness) as a proxy for _lifetime_ (likelihood of not-dangling).
But value category is not lifetime!

----

Dangling references are one of the big unsolved problems in C++ programming. It's an unsolved problem
for a reason: there is no silver bullet. In particular, _adding `=delete`d `const&&`-qualified overloads_ is not
going to magically solve your dangling-reference problem. Adding those deleted overloads is going to
make your life *more difficult* by disabling perfectly reasonable code such as our `print_it` example,
and it's going to introduce a false sense of security while continuing silently to permit buggy code
such as our `make_iterator` example.

Guideline: Do not assume that "rvalues are short-lived," nor that "everything sufficiently long-lived must be an lvalue."
Vice versa, do not assume that "lvalues are long-lived," nor that "everything sufficiently short-lived must appear as an rvalue."

> Value category is not lifetime.

It's a mantra worth hanging onto!

----

See also [Abseil Tip of the Week #149: "Object Lifetimes vs. `=delete`"](https://abseil.io/tips/149) (May 2018).
