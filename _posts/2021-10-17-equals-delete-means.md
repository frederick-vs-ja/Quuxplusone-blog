---
layout: post
title: "What `=delete` means"
date: 2021-10-17 00:01:00 +0000
tags:
  argument-dependent-lookup
  c++-learner-track
  library-design
excerpt: |
  In my C++ training classes, I often explain the meaning of `=delete` as

  > The library author is saying, "I know what you're trying to do,
  > and what you're trying to do is wrong."
---

In my C++ training classes, I often explain the meaning of `=delete` as

> The library author is saying, "I know what you're trying to do,
> and what you're trying to do is wrong."

In some situations there's not much difference between
`=delete`’ing a function and not-`=delete`’ing it. In fact, the most
common way that people first learn about `=delete` is in a context where
it doesn't really matter — which is why they start out a little bit
confused!

    struct MoveOnly {
        MoveOnly(MoveOnly&&);
        MoveOnly& operator=(MoveOnly&&);

        MoveOnly(const MoveOnly&) = delete;  // redundant
        MoveOnly& operator=(const MoveOnly&) = delete;  // redundant
    };

The question of whether to explicitly `=delete` here, or just leave the
copy members undeclared, is purely stylistic. Me personally, I'd
leave them undeclared and save two lines of code. Someone else might
err on the side of being explicit.

Where `=delete` really matters is when it's the best match
in an overload set with at least one other candidate. My go-to example
for this is `std::cref`, which takes a `const T&` and turns it into
a `reference_wrapper<const T>`. Recall that [`const T&` is happy to
bind to both lvalues and rvalues.](/blog/2022/02/02/look-what-they-need/)
So, if the library designer provides only one overload of `std::cref`,
then it'll accept both lvalues and rvalues:

    template<class T>
    auto cref(const T&) -> std::reference_wrapper<const T>;

    int i = 42;
    auto r1 = std::cref(i);   // OK
    auto r2 = std::cref(42);  // OK but dangling!

When the STL's designers added a deleted overload for `const T&&`, they were saying,
"I know what you're trying to do [namely, call `std::cref` on an rvalue],
and what you're trying to do is wrong."

    template<class T>
    auto cref(const T&) -> std::reference_wrapper<const T>;

    template<class T>
    auto cref(const T&&) = delete;

    auto r2 = std::cref(42);  // Error, best match is deleted

This is not the same as the author saying "I don't know what you're trying to do."
It's not even the same as the author saying "I know what you're trying to do, but I
don't do it; maybe somebody else might." In those cases, the author might leave
the function undeclared, or have it SFINAE away. With `=delete`, the library author
is specifically stating that for the thing you're trying to do,
[the buck stops with them](https://www.trumanlibrary.gov/education/trivia/buck-stops-here-sign) —
and guess what? They don't want you to do it.

----

There are very few uses of `=delete` in the STL. They tend to get used in three
main situations:

First, of course, there's deleting the copy operations of types you want to make
immobile, such as `lock_guard` and `std::pmr::monotonic_buffer_resource`.

Second, there's the ones isomorphic to `std::cref` — the STL loves to use "rvalueness" as a proxy
for "short-livedness." Sometimes, as with `std::cref` and `std::addressof`, I think
it's a defensible design choice, good enough to use as an example in class.
Sometimes, as with
[the constructor of `regex_iterator`](https://en.cppreference.com/w/cpp/regex/regex_iterator/regex_iterator),
I think it's just silly: I don't use that as an example.
For more on this topic, see
["Value category is not lifetime"](/blog/2019/03/11/value-category-is-not-lifetime/) (2019-03-11).

A similar situation arises with `nullptr`; for example, `std::string_view` deletes
its constructor and assignment operator from `nullptr_t`:

    std::string_view sv1 = nullptr;   // Error, best match is deleted
    std::string_view sv2 = (char*)0;  // OK, UB at runtime

Perhaps the most interesting and defensible case is C++20's deleted overloads of `<<`
for "character types" that aren't `char`. [Godbolt](https://godbolt.org/z/Pnforv1n4):

    std::cout << u8"hello world!" << '\n';  // const char8_t*
    std::cout << L'h' << '\n';              // wchar_t

Using C++17's overload set, which had no overloads for `const char8_t*` nor `wchar_t`,
this example would just choose the best-matching candidates: `ostream::operator<<(const void*)` and
`ostream::operator<<(int)` respectively. In C++20, `ostream` provides specifically `=delete`’d overloads
for these types, so you get a nice compiler error instead of gibberish at runtime. This is the
library designer saying, "I understand what you're trying to do [print a `wchar_t` to a narrow-character
terminal], and what you're trying to do is wrong."

> Now, maybe the designer actually does want you to be able to do it, someday! But they know
> that if you try it this year, you'll just end up with gibberish and tears. So it's best,
> instead of allowing the less-good `const void*` or `int` overloads to be picked, to shut you
> down at compile time. `=delete` provides the mechanism to do just that.

The third and most interesting use of `=delete` in the STL is to disable specific template arguments.
For example, the STL provides bodies for `std::make_unique<Widget>` and `std::make_unique<Widget[]>`, but if you
try `std::make_unique<Widget[10]>` you'll hit a deleted overload.
"I know what you're trying to do, and you're wrong." (You probably wanted
`std::make_unique<Widget[]>(10)` instead.)

----

Finally, although the STL doesn't do this anywhere yet, there's one more place you might consider using `=delete`:
If your library provides a function that's meant to be called through ADL, and then you deprecate and remove
that function. [Godbolt](https://godbolt.org/z/9bxaGGc8n):

    namespace mine {
        struct Widget {};
        void oldapi(Widget);
        void newapi(Widget);
    } // namespace mine

    struct Sink { Sink(auto) {} };
    int oldapi(Sink);

    int main() {
        mine::Widget w;
        oldapi(w);
    }

We can mark `oldapi(Widget)` as `[[deprecated("please switch to newapi")]]` for a few releases,
but at some point we'll want to get rid of it. We could just remove the declaration altogether;
if we do that, then any code (such as `main`) that's still calling `oldapi` via ADL might silently
switch to some other `oldapi`, such as `oldapi(Sink)`. So, if we're really worried about this
possibility, then we might change the declaration of our `oldapi` from `[[deprecated]]` to `=delete`’d.
Again, this is us saying, "I know what you're trying to do, `main`, and you're (now) wrong to
want to do it."

Notice that this logic doesn't apply if we expect the caller to always call `mine::oldapi`
via its qualified name. The compiler is perfectly capable of telling the programmer that the
qualified name `mine::oldapi` doesn't exist (anymore). The programmer's intent
in that case is very clear; the compiler doesn't need any help from the library designer. Where `=delete`
is useful is where the library designer sees a specific pattern of (potential) misuse: in the same
way that we craft positive overload sets to say "I know what this caller wants, and I'm implementing
the best way to do that thing," `=delete` allows us to craft "negative" overload sets to say
"I know what this caller wants, and _I don't want them to be doing that._"

----

See also:

* ["Value category is not lifetime"](/blog/2019/03/11/value-category-is-not-lifetime/) (2019-03-11)

* ["C's removal of `gets` could have been done better"](/blog/2021/03/12/gets-considered-harmful-duh/) (2021-03-12)

* ["Perfect forwarding call wrappers need `=delete`"](/blog/2021/07/30/perfect-forwarding-call-wrapper/) (2021-07-30)
