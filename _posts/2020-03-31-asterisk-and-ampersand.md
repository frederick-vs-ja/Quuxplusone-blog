---
layout: post
title: 'When is `*x` also `&x`?'
date: 2020-03-31 00:01:00 +0000
tags:
  c++-learner-track
  lambdas
  slogans
---

Via [Ben Antonellis on Code Review StackExchange](https://codereview.stackexchange.com/questions/239381/implementing-any-of-in-c/):

    template<class F>
    void call_twice(F *f) {
        (*f)();
        (*f)();
    }

    static void one() {
        puts("one");
    }
    auto two = []() {
        puts("two");
    };

    int main() {
        call_twice(one);  // OK
        call_twice(two);  // ERROR - two is not a pointer
    }

Okay, so let's make `two` a pointer...

        call_twice(*two);  // OK

Wait... _what?_

----

Learners of C and C++ frequently have trouble with pointers — particularly
the difference between `&` and `*` — particularly in C++, where both `&` and
`*` can appear in declarations as well as in expressions. My tricks for
overcoming this difficulty include:

- <b>Teach pointers first.</b> In C++ we want to build the intuition that
    every object has an "address" in memory. The idea that we can represent
    memory addresses via the type system, just like we can represent natural
    numbers or Vehicles, is a hugely satisfying leap of logic.
    Teach references approximately five minutes later.

- Teach the logic behind C-style declarations. `int *p` can mean "`p` is an `int*`,"
    and it can equally well mean "`*p` is an `int`." `int const *f()` can mean
    "You aren't allowed to modify the `int`," or it can equally well mean
    "You aren't allowed to modify `*f()`." (Of course this only works
    for pointers, not references; that's another reason it's important to reveal
    pointers early.)

- "Follow the shooting star." In an expression, both `*` and `->` indicate that
    you're following a pointer — you already _have_ a pointer, and you're
    _following_ it to see what's at the other end.

So in the code above, it's unsurprising to see someone write `two`, have it fail
to compile because `could not match 'F *' against 'lambda'`, and then see them
try to fix the bug by adding a star — `*two` — instead of an ampersand.
What's surprising is that adding the star _works._

----

If you've seen my talk ["Generic Lambdas From Scratch,"](https://www.youtube.com/watch?v=3jCOwajNch0)
you know that a lambda is just syntactic sugar for a struct with a member `operator()`.
When a lambda has no captures, then C++ gives the underlying struct one more method:
a _conversion function_ to the appropriate function pointer type. Our `two` is basically
equivalent to the following struct:

    using FuncPtr = void(*)();
    struct TwoType {
        static void behavior() { puts("two"); }
        void operator()() const { behavior(); }
        operator FuncPtr() const { return &behavior; }
    };
    auto two = TwoType();

Usually we trigger this conversion to function pointer by doing something to
the lambda that requires a scalar type. Commonly, idiomatically, the "coerce to scalar type"
operator is unary `+`. (Unlike unary `-`, unary `+` works just fine on pointer types.)

    auto pf = +[]() { puts("two"); };
    static_assert(std::is_same_v<FuncPtr, decltype(pf)>);

----

You should also know that in expressions, functions very eagerly decay into function _pointers_,
just as arrays eagerly decay into pointers-to-their-first-elements.
For example, instead of `return &behavior` I could have written `return behavior`: the function
`behavior` is happy to implicitly convert from `void()` to `void(*)()`.

The unary `*` operator takes a function pointer and dereferences it to give me a function...
which will then eagerly decay back to a function pointer if I do just about anything with it,
including `*`-ing it again! So all of the following are equivalent:

    void f();
    auto pf1 = &f;
    auto pf2 = f;
    auto pf3 = *f;
    auto pf4 = *******f;

In the initializer of `pf4`, `f` implicitly converts to a scalar function pointer so that
we can `*` it; that gives us a reference to a function, which implicitly converts back to a
function pointer so that we can `*` it the second time; and so on, arbitrarily many times.

----

That's what happens in Ben's code at the top of this post.

    template<class F>
    void call_twice(F *f);

    static void one() { puts("one"); }
    auto two = []() { puts("two"); };

    int main() {
        call_twice(*one);  // OK
        call_twice(*two);  // OK
    }

Unary `*` can't apply directly to a function such as `one`, so `one` implicitly converts to a function pointer
so that we can `*` it. That gives a function reference. When we pass that function reference by value
to `call_twice`, it decays back to a function pointer.

Unary `*` can't apply directly to a lambda such as `two`, so `two` implicitly converts to a function pointer
so that we can `*` it. That gives a function reference. When we pass that function reference by value
to `call_twice`, it decays to a function pointer.

The upshot is that `call_twice` is only ever instantiated once, with `F = void(*)()`.
[Godbolt:](https://godbolt.org/z/6imnU6)

    int instantiations = 0;

    template<class F>
    void myTemplate(F *f) {
        static int x = ++instantiations;
        printf("Instantiation %d: ", x); (*f)();
    }

    int main() {
        auto a = []() { puts("A"); };
        auto b = []() { puts("B"); };
        myTemplate(&a);  // Instantiation 1: A
        myTemplate(&b);  // Instantiation 2: B
        myTemplate(*a);  // Instantiation 3: A
        myTemplate(*b);  // Instantiation 3: B
    }

Anyway, I thought this was an interesting tidbit.
