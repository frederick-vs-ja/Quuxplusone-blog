---
layout: post
title: '"Universal reference" or "forwarding reference"?'
date: 2022-02-02 00:01:00 +0000
tags:
  c++-learner-track
  cpplang-slack
  holy-wars
  memes
excerpt: |
  Two convergent observations from my corner of the C++ world:

  <b>1.</b> Multiple book authors pushing the idea that Scott Meyers' original
  phrase "universal reference" (for `T&&`) is actually _preferable_
  to the now-Standard term "forwarding reference."

  <b>2.</b> Multiple C++ learners (or perhaps intermediate-level C++ programmers
  having a senior moment) asking whether there's an easy way to pass an _rvalue_
  expression to a function expecting a const _lvalue_ reference.
---

Two convergent observations from my corner of the C++ world:

<b>1.</b> Multiple book authors pushing the idea that Scott Meyers' original
phrase "universal reference" (for `T&&`) is actually _preferable_
to the [now-Standard](https://timsong-cpp.github.io/cppwp/n4861/temp.deduct#call-3.sentence-3)
term "forwarding reference." For example,
Nico Josuttis, in [_C++ Move Semantics: The Complete Guide_](https://leanpub.com/cppmove):

> The important feature of universal references is that they can
> bind to objects and expressions of any value category.

And someone else, elsewhere:

> [The name "universal reference"] perfectly describes what these references represent:
> A reference to anything. A _universal_ reference.

<b>2.</b> Multiple C++ learners (or perhaps intermediate-level C++ programmers
having a senior moment) asking whether there's an easy way to pass an _rvalue_
expression to a function expecting a const _lvalue_ reference.

These latter programmers have somehow internalized the advanced notion that
"lvalue references don't bind to rvalues (and rvalue references don't bind to lvalues)"
without first learning that `const X&` binds to everything!

----

C++98 didn't have rvalue references, only "references" — what we now call
"lvalue references." The rule was simply that a mutable reference would bind only to
a mutable lvalue, but a const reference could bind to anything.

    void change(int&);
    void observe(const int&);

    int main() {
        int i = 41;
        change(i);    // OK, lvalue
        change(42);   // Error, rvalue
        observe(i);   // OK, lvalue
        observe(42);  // OK, rvalue
    }

This applies even when that type `int` comes from type deduction:

    template<class T>
    void observe(const T&);

    int main() {
        int i = 41;
        const int ci = 42;
        observe(i);   // OK, lvalue, T=int
        observe(ci);  // OK, lvalue, T=int
        observe(43);  // OK, rvalue, T=int
    }

When C++11 invented rvalue references, none of this behavior changed at all.
`const T&` still binds happily to both lvalues and rvalues.

> `const T&` is the O.G. universal reference.

C++11 also invented the forwarding reference: that when there's a deduced type `T`
directly modified by `&&`, `T` can sometimes be deduced as an lvalue reference type
(even though this never happens anywhere else in the language).

    template<class T>
    void forward(T&&);

    int main() {
        int i = 41;
        const int ci = 42;
        forward(i);   // OK, lvalue, T=int&
        forward(ci);  // OK, lvalue, T=const int&
        forward(43);  // OK, rvalue, T=int
    }

The advantage of `T&&` is that, by looking at whether `T` deduced as a reference type,
you can tell whether your caller considered the argument an lvalue or an rvalue.
That's not useful information in its own right; it is useful only if you are planning
to _forward_ your argument as its original value category — lvalues as lvalues, rvalues
as rvalues. That's what `std::forward<T>(t)` is for.

> If you see code using `std::forward<T>` without an originating `T&&`, it's almost certainly
> buggy. If you see code using (deduced) `T&&` without `std::forward<T>`, it's either buggy
> or it's C++20 Ranges. (Ranges [ill-advisedly](/blog/2019/03/11/value-category-is-not-lifetime/)
> uses value category to denote lifetime rather than pilferability, so Ranges code
> tends to forward rvalueness much more conservatively than ordinary C++ code does.)

In exchange for this advantage — forwardability — you pay in template bloat. Notice that
we get three different instantiations of `void forward(T&&)` above, whereas we got only
a single template instantiation of `void observe(const T&)`.

![](/blog/images/2022-02-02-look-what-they-need.jpg){: .meme}

Forwarding references should generally be used only where there's an actual need for them;
they shouldn't be the first tool you reach for.

And "forwarding reference" is _absolutely_ the correct name for forwarding references.

----

See also:

* ["Don't blindly prefer `emplace_back` to `push_back`"](/blog/2021/03/03/push-back-emplace-back/) (2021-03-03)
