---
layout: post
title: "Guard nouns or guard verbs?"
date: 2023-11-11 00:01:00 +0000
tags:
  attributes
  concurrency
  exception-handling
  kona-2023
  library-design
  mailing-list-quotes
  proposal
---

One particular tension between two design idioms comes up over and over (at least
in C++). When we have a property we're trying to enforce on the way some _thing_
is _used_, such that we need to annotate some part of the code, should we annotate
the _thing_ we're using, or should we annotate our _uses_ of the thing?

> If you haven't read Steve Yegge's ["Execution in the Kingdom of Nouns"](https://steve-yegge.blogspot.com/2006/03/execution-in-kingdom-of-nouns.html)
> (March 2006), it's worth doing so now; though it's not really relevant
> to this blog post per se.

The most visible example of this in modern C++ is C++20's [`atomic_ref`](https://en.cppreference.com/w/cpp/atomic/atomic_ref),
which provides an "access-centered" counterpart to C++11's [`atomic`](https://en.cppreference.com/w/cpp/atomic/atomic).
Suppose the programmer wants to make two threads communicate by atomic loads and stores on a global variable.
He has two choices:

    // Choice 1
    std::atomic<int> ga = 0;
    void producer() {
        ga.store(42);
    }
    int consumer() {
        return ga.load();
    }

    // Choice 2
    int g = 0;
    void producer() {
        std::atomic_ref<int>(g).store(42);
    }
    int consumer() {
        return std::atomic_ref<int>(g).load();
    }

Choice 1 makes sense if you're sure that _every_ access to `ga` will need to be atomic: it prevents your
ever performing a non-atomic access by accident. But Choice 2 makes sense if your atomic accesses are part of a carefully
crafted and compartmentalized algorithm, such that accesses to `g` elsewhere in the program _don't_ need atomicity.

Guard the _noun_ (the variable `ga`), or guard the _verb_ (the codepaths that access it)?

----

Choice 2 (guard the verb) is the clear winner in the other place I've seen this particular tension before:
Linus Torvalds' attitude toward `volatile`. Bear in mind that the following pull-quotes are from the mid-2000s —
about 20 years ago, certainly before C came out with `_Atomic` and all that — so by `volatile` here
we mean essentially what `std::atomic` meant above.

Linus, [2006-07-06](https://yarchive.net/comp/volatile.html):

> [F]or accesses that have some software rules (ie not IO devices etc),
> the rules for "volatile" are too vague to be useful.

(See ["`volatile` means it really happens"](/blog/2022/01/28/volatile-means-it-really-happens/) (2022-01-28).)

> So if you actually have rules about how to access a particular piece of
> memory, just make those rules _explicit_. Use the real rules. Not
> volatile, because volatile will always do the wrong thing.
>
> Also, more importantly, "volatile" is on the wrong _part_ of the whole
> system. In C, it's "data" that is volatile, but that is insane. Data
> isn't volatile - <em>accesses</em> are volatile. So it may make sense to say
> "make this particular <em>access</em> be careful", but not "make all accesses to
> this data use some random strategy".

Linus five years earlier, [2001-07-23](https://yarchive.net/comp/volatile.html):

> "volatile" (in the data sense, not the "asm volatile" sense) is almost
> always a sign of a bug. If you have an algorithm that depends on the
> (compiler-specific) behaviour of "volatile", you need to change the
> algorithm, not try to hide the fact that you have a bug.
>
> Now, the change of algorithm might be something like
>
>     /*
>      * We need to get _one_ value here, because our
>      * state machine ....
>      */
>     unsigned long stable = *(volatile unsigned long *)ptr;
>
>     switch (stable) {
>     ....
>     }
>
> where the volatile is in the place where we care, and the volatile is
> commented on <em>why</em> it actually is the correct thing to do.

UPDATE: And here's Linus twenty years later, [2024-03-25](https://lwn.net/ml/linux-kernel/CAHk-=wjP1i014DGPKTsAC6TpByC3xeNHDjVA4E4gsnzUgJBYBQ@mail.gmail.com/),
as quoted in ["A memory model for Rust code in the kernel"](https://lwn.net/SubscriberLink/967049/0ffb9b9ed8940013/) (April 2024):

> A variable may be entirely stable in some cases (i.e. locks held), but not in others.
>
> So it's not the <em>variable</em> (aka "object") that is 'volatile', it's the
> <em>context</em> that makes a particular access volatile.

----

This tension between "guarding the noun" and "guarding the verb" is also relevant
to [P2946R0 "A flexible solution to the problems of `noexcept`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2946r0.pdf)
(Pablo Halpern, 2023) — seen at the Kona meeting this past week — which proposes an attribute `[[throws_nothing]]`
reflecting more or less what the Standard Library means by ["_Throws:_ Nothing."](https://eel.is/c++draft/string.access#3)
We can add it to a function that (because of the [Lakos Rule](/blog/2018/04/25/the-lakos-rule/))
is non-`noexcept`, but which never actually throws: we're willing to promise that to the compiler
(if perhaps not to the client programmer [forever](https://adamj.eu/tech/2021/11/03/software-engineering-is-programming-integrated-over-time/)).
So for example we could annotate

    struct V {
        int& at(int); // may throw
        int& uat(int) [[throws_nothing]];
        int& nat(int) noexcept;
    };

The difference between `[[throws_nothing]]` and `noexcept` is that while both of them tell the optimizer
"I don't throw exceptions," the keyword `noexcept` means "I have no preconditions — I have a wide contract"
and participates in the type system:

    static_assert(!std::same_as<decltype(&V::nat), decltype(&V::at)>);
    static_assert(noexcept(V().nat(42)));

whereas `[[throws_nothing]]` is a pure optimization hint and does not:

    static_assert(std::same_as<decltype(&V::uat), decltype(&V::at)>);
    static_assert(!noexcept(V().uat(42)));

P2946R0 proposes annotating only function declarations. At Kona there was some discussion of whether
the attribute should apply to the function _type_ instead, [like `[[gnu::format]]`](https://godbolt.org/z/v3he97osK),
so that we could write:

    int [[throws_nothing]] (*fp)();

    fp();  // compiler knows this call won't throw

Both of these are examples of annotating the noun. It was pointed out that sometimes what we want
is to annotate the verb instead:

    struct S {
        int (*fp)();
        bool points_to_nonthrowing;
    };

    void f(S s) {
      std::vector<int> v;
      if (s.points_to_nonthrowing) {
        s.fp(); // A
      } else {
        GuardObject guard;
        s.fp();
        guard.complete();
      }
    }

On the line marked `A`, GCC [generates](https://godbolt.org/z/qcGMfo7Ev) an ordinary indirect call,
with extra code to destroy `v` (and adjust the stack) in case `s.fp()` throws. If we could convince
the compiler that `s.fp()` couldn't throw, then
[it could become a tail-call](/blog/2022/07/30/type-erased-inplace-printable/) instead.
We could simply lie to the compiler, like this:

      if (s.points_to_nonthrowing) {
        ((int(*)()noexcept)(s.fp))(); // A
      }

But that's both ugly and (formally) UB. The obvious attempt is:

      if (s.points_to_nonthrowing) {
        try {
          s.fp();
        } catch (...) { __builtin_unreachable(); }
      }

But GCC's optimizer, unsurprisingly, can't figure that one out. Maybe it would be
useful to have a way of annotating the verb: "`fp` is throwing in general, but along this
specific codepath, I promise that this line of code won't throw."

    [[throws_nothing]] s.fp();

> Coincidentally, another paper seen at Kona this week — Giuseppe D'Angelo's
> [P2992R0 "Attribute `[[discard]]`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2992r0.html) —
> tentatively proposed that C++'s grammar should be modified to permit attributes on subexpressions.
> That's what we'd want for this case, really. The trouble is that the grammar for that
> is crazy ugly: an attribute prefixed to the line, as above, applies to the _statement_,
> so an attribute applied to the _expression_ would have to be postfix. That ugliness, I think,
> will prevent C++ from ever getting attributes-on-expressions.

So again we have two competing idioms here: apply `[[throws_nothing]]` to
the _noun_ (the function or pointer), or to the _verb_ (the codepath that calls it)?
Neither approach is strictly better than the other; they both have some appeal.

The next question is: Do we want the paper standard to
favor one of these idioms over the other, before any compiler vendor has implemented either
of them? I think strongly not — but I'm eager to see some compiler vendor produce an implementation
of one or the other, or both, so that we can get some real-world experience with them.

I'd also be thrilled to see any compiler's optimizer taught to understand the
`catch (...) { __builtin_unreachable(); }` pattern above. That could be done without
any language changes, as a pure middle-end optimization.
