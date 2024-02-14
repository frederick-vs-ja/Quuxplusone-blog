---
layout: post
title: 'Feature requests for the `Auto` macro'
date: 2024-02-14 00:01:00 +0000
tags:
  exception-handling
  lambdas
  preprocessor
---

From time to time readers send me "feature requests" for the `Auto` scope-guard macro
(["The `Auto` macro"](/blog/2018/08/11/the-auto-macro) (2018-08-11)).
Usually, I say "No need!" The neat thing about `Auto`'s particular syntax
is that it's conceptually just a way to defer code to the end of a scope. Feature requests
usually take the form of modifying the deferred code in some way — which is already (and more
explicitly) allowed simply by... writing the code that way.

Before we look at those "not-a-bug" examples, let's see the one feature request I've
actually agreed with and adopted:

## Throwing from deferred code

"My deferred code might throw, which smacks into the implicit `noexcept` of the `Auto`
object's destructor and terminates. That destructor should be `noexcept(false)`!"

Quite true! Instead of the internal lambda-holder looking like this:

    template<class L>
    class AtScopeExit {
      L& m_lambda;
    public:
      AtScopeExit(L& action) : m_lambda(action) {}
      ~AtScopeExit() { m_lambda(); }
    };

its destructor should really look like this:

      ~AtScopeExit() noexcept(false) { m_lambda(); }

That explicit `noexcept(false)` undoes the implicit `noexcept` that C++ places
on most destructors.

> Formally, noexceptness is propagated from the bases and data
> members into the destructor's implicit noexcept-spec
> ([[except.spec]/8](https://eel.is/c++draft/except.spec#8));
> in other words, given `struct Y { X x; ~Y() {} }`, `~Y` will have the same
> noexceptness as `~X` even though `~Y() {}` is user-provided. This is different from how it works for, say,
> move constructors, where a user-provided `Y(Y&&) {}` is implicitly non-noexcept
> even if `X(X&&)` is noexcept.

Adding `noexcept(false)` to the internal type's destructor
allows us to support code like this ([Godbolt](https://godbolt.org/z/qG1aM7fjM)):

    void example1() {
      Auto(throw 42);
      if (cond)
        return; // here 42 is thrown
      neverThrows();
      // here 42 is thrown
    }

Of course, if we're exiting the scope because of an exception, and then the `Auto`'s code
throws its own exception, the runtime will call `std::terminate` anyway (because you can't
propagate two exceptions at once). In that case, our addition of `noexcept(false)` is
harmless but not helpful either.

Removing that implicit `noexcept` from the destructor arguably alters the meaning of
([Godbolt](https://godbolt.org/z/va7bzWbE1)):

    void cleanup();
    void test() {
      Auto(cleanup());
    }

Before, `test` couldn't ever propagate an exception; if `cleanup` threw, it would smack
into the destructor's implicit `noexcept` and terminate. So there was only one way to
exit from `test`. After, there are two ways to exit, so the behavior isn't as simple —
but GCC 13 generates the same text section for both (pushing the whole difference
into the unwind tables), and Clang 17 actually generates _smaller_ code for the new `Auto`!
Anyway, if this matters to you, you can generate even smaller code — identical before and
after this change — by declaring `void cleanup() noexcept`.

Now let's see some "rejected" feature requests.

## Detecting in-flight exceptions

"My deferred code might throw, which terminates if there's already an exception in flight.
Therefore, `Auto`'s lambda should wrap my code in `try`/`catch`!"

"My deferred code might throw, which terminates if there's already an exception in flight.
Therefore, `Auto`'s lambda should wrap my code in an `if`, so it won't run if there's an
exception in flight!"

The `Auto`-world answer is that _you're_ responsible for what code runs at the end of your
scope; if you want a particular control-flow construct, just write it! Like this:

    void example2() {
      Auto(
        try {
          mightThrowC();
        } catch (...) {}
      );
      mightThrowA();
      mightThrowB();
    }

    void example3() {
      int inflight = std::uncaught_exceptions();
      Auto(
        if (std::uncaught_exceptions() == inflight) {
          mightThrowC();
        }
      );
      mightThrowA();
      mightThrowB();
    }

But in most cases such code isn't needed.
`Auto`'s simple definition ensures that "you don't pay for what you don't use."

## Explicit `commit`

"I should be able to name the `Auto` object and say `guard.commit()` at the end of my
transaction; only uncommitted transactions should run the deferred code!"

The `Auto`-world answer is that conceptually there _is_ no "`Auto` object";
`Auto` simulates a control-flow construct, not an object with state.
Instead of something like this:

    void fantasy4() {
      FantasyTransactionGuard g(
        puts("Transaction failed");
      );
      mightThrowA();
      if (rand()) return;
      mightThrowB();
      g.commit(); // now the puts won't run!
    }

in `Auto`-world you'd write the boolean variable explicitly in _your_ code:

    void example4(int k, int v) {
      bool committed = false;
      Auto(
        if (!committed) {
          puts("Transaction failed");
        }
      );
      mightThrowA();
      if (rand()) return;
      mightThrowB();
      committed = true; // now the puts won't run!
    }

## Capture by value

"`Auto`'s lambda captures everything by `[&]`, but I need a version that captures
by value instead!"

The `Auto`-world answer is that there _is_ no "lambda"; `Auto` simulates
a control-flow construct, not an object with state. If you need a copy of
some variable `i`, just make a copy! Instead of something like this:

    void fantasy5() {
      static int counter = 0;
      FantasyAutoCapturingByValue(
        printf("finished operation %d\n", counter);
      );
      counter += rand();
      // here the original value is printed
    }

in `Auto`-world you'd write the copy operation explicitly in _your_ code:

    void example5() {
      static int counter = 0;
      int originalCounter = counter;
      Auto(
        printf("finished operation %d\n", originalCounter);
      );
      counter += rand();
      // here the original value is printed
    }

This keeps all the important control flow and data-copying visible in _your_ code,
while hiding only the very smallest amount of "magic" behind the macro.
