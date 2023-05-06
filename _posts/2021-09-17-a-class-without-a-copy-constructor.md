---
layout: post
title: 'A class without a copy constructor'
date: 2021-09-17 00:01:00 +0000
tags:
  concepts
  cpplang-slack
  implementation-divergence
  implicit-move
excerpt: |
  Here's a puzzle for you: How do you write a class without a copy constructor?
---

Here's a puzzle for you: How do you write a class without a copy constructor?
You might answer:

    struct A {};

But no: `A` has a copy constructor. The compiler has generated a _defaulted_
copy constructor for `A`. You might try again like this:

    struct B { B(const B&) = delete; };

But no again! `B` has a copy constructor. That copy constructor is _deleted_, but
it's still present. You can tell it's present, because if you use `B` in
a situation where overload resolution would consider both the copy constructor
and some other (less desirable) candidate, the deleted copy constructor will
still win out:

    struct FromB { FromB(const B&); };
    void f(B);
    void f(FromB);
    void test(const B& b) { f(b); }

    [...]
    error: call to deleted constructor of 'B'
        void test(const B& b) { f(b); }
                                  ^

Here's the answer I have in mind. Starting in C++20, we can use Concepts
to make classes that (almost) do not have copy constructors. For example:

    template<class T>
    struct C {
        C(const C&) requires (sizeof(T) != 1);
    };

Now, technically, `C` has a copy constructor: our user-provided
one. Since it has a user-provided copy constructor, the compiler does not
generate any implicitly defaulted copy constructor for it.
However, when `T=char`, our user-provided copy constructor is not
[_eligible_](https://eel.is/c++draft/special#6), because its constraint is
not satisfied. So, `C<char>` has no implicitly defaulted copy constructor
_and_ its existing copy constructor is not eligible. We can observe
the change in behavior on MSVC ([Godbolt](https://godbolt.org/z/bYr8YhvjT)):

    struct FromC { FromC(const C<char>&); };
    void f(C<char>);
    void f(FromC);
    void test(const C<char>& c) {
        f(c);
            // C<char> has no copy constructor, so
            // f(FromC) is the only viable candidate
    }

As of this writing, neither GCC nor Clang implements this scenario
the way I'd expect them to: they both continue to think that
`void f(C<char>)` is the best-matching candidate, despite there
being no implicit conversion sequence from `const C<char>&` to
`C<char>`.

All three compilers agree that if you give `C` additional
implicit conversions to and from `Other`, then there _will_ be
an implicit conversion sequence from `const C<char>&` to `C<char>`
— it just goes via `Other`!
This leads to an amusing situation where the compiler will actually
pick an implicit conversion sequence that takes the scenic route
"past" another, worse-matching, candidate overload before circling
back to the better-matching overload ([Godbolt](https://godbolt.org/z/ExThWEM87)):

    struct Other {};

    template<class T>
    struct C {
        C(const C&) requires (sizeof(T) != 1);
        operator Other() const;
        C(const Other&);
    };

    void f(C<char>);
    void f(Other);
    void test(const C<char>& c) {
        f(c);
            // equivalent to f(C<char>(Other(c))),
            // even though f(Other(c)) would have
            // had a shorter conversion sequence
    }


## Relevance to "implicit move"

In C++11, C++14, and C++17, the rule for `return x;`
was to first do overload resolution with `x` as an rvalue,
looking specifically for constructors taking type `X&&`;
and then if that failed, redo the overload resolution treating
`x` as an lvalue (and looking for all viable conversion sequences).

In C++20, my [P1155 "More implicit moves"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1155r3.html)
changed the rules so that the first overload resolution would
look for _any_ conversion sequence. This allowed us to pick up
constructors taking `BaseOfX&&`, converting constructors taking
`X` by value, etc., in the first overload resolution pass.
This produced a similarly counterintuitive effect, because
it has always been easy to create a class with no _move_ constructor!

    struct D {
        D(D&);
        operator Other() const;
        D(Other);
    };

    D rf(D d) {
        return d;
    }

In C++17, this would fail the first overload resolution and end up
calling `D(D&)`. In C++20, the first overload resolution succeeds
because "convert `D` to `Other`, then convert `Other` to `D`" is
considered a usable implicit conversion sequence. (I'm honestly not
sure _why_ it's considered usable; I've always thought C++ had a
general rule that you could never stack user-defined conversions
more than 1 deep. But that "rule" seems to be implemented as
[a laundry list of special cases in [over.best.ics.general]](https://eel.is/c++draft/over.best.ics#general-4)
rather than a blanket prohibition.)

The C++20 rule change has generally been treated by compiler vendors
as a "[DR](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#dr)" —
i.e., they've backported it into earlier modes as well —
because it tends to give better codegen and also to be easier to
implement. So for any given compiler release, you won't see any
difference between `-std=c++17` and `-std=c++20` on the above
snippet. But comparing different releases of the same compiler,
you'll see a point in time where it switches from the old behavior
to the new behavior in _all_ modes. For MSVC, this has already happened,
[in MSVC 19.24](https://godbolt.org/z/x6783T1Ez).
For Clang, the switchover is happening in Clang 13.
I don't know about GCC.

----

Another interesting ramification of the new rules is that "copy elision"
can now elide things that aren't copy or move constructors. For example,
suppose we rewrite the function above as

    D rf2() { D d; return d; }

- If copy elision doesn't happen, we get a call to `D::operator Other() const`
    and a call to `D(Other)`.

- If copy elision happens, then we get nothing.

----

I tentatively imagine that CWG might want to add a new bullet point to the
laundry list in [over.best.ics.general], to forbid stacking user-defined
conversions in `return` statements. Such a change would temporarily restore the
C++11 behavior of `rf(D)` above — but only until
[P2266 "Simpler implicit move"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2266r1.html)
got rid of the second overload resolution pass altogether, at which point
`rf(D)` would become ill-formed (and we'd all probably breathe a sigh of relief).

----

UPDATE, 2021-09-20: I still think the ability to make a class with no
(eligible) copy constructor is new-in-C++20, but Lénárd Szolnoki sends me
[this example in pure C++98](https://godbolt.org/z/Tzfr6T569) proving
that the "scenic route" of stacking user-defined conversions has been
possible since the dawn of time.

    struct H;
    struct G {
        G(G&);
        G(const H&);
    };

    struct H { H(const G&); };

    void f(G);
    void f(H);

    void test(const G& g) {
        f(g);  // equivalent to f(G(H(g)))
    }
