---
layout: post
title: 'A quirk of qualified member lookup'
date: 2020-01-11 00:02:00 +0000
tags:
  cpplang-slack
  implementation-divergence
  language-design
  name-lookup
  templates
  today-i-learned
---

Yesterday on the cpplang Slack, someone posted [this bizarre MSVC behavior](https://godbolt.org/z/NGA9FQ):

    namespace A { int i; }
    namespace B { struct S {}; }

    void test() {
        B::S bs;
        bs.A::i = 42;
    }

MSVC accepts this invalid C++ code, and — even weirder — seems to enter a mode in which no matter
what you _do_ with `bs.A::i`, it doesn't generate any actual machine code. Okay, so another day, another
MSVC parser bug, right?

But then Momchil Velikov pointed to some "weird-looking text" in
[[basic.lookup.classref]/4](http://eel.is/c++draft/basic.lookup.classref#4):

> If the id-expression in a class member access is a _qualified-id_ of the form
> `class-name-or-namespace-name::...`
> the _class-name-or-namespace-name_ following the `.` or `->` operator is first
> looked up in the class of the object expression and the name, if found, is used.
> Otherwise it is looked up in the context of the entire _postfix-expression_.

This wording doesn't explain MSVC's wacky parser behavior, but it _is_ news to me.
It deals with cases like the following ([Godbolt](https://godbolt.org/z/ecuhLY)):

    struct A { void foo(); };
    struct B { void foo(); };
    struct C : A, B { void foo(); };
    struct D : A, B { using T = B; void foo(); };

    void one(C *c) {
        using T = A;
        c->T::foo();  // A::foo
    }

    void two(D *d) {
        using T = A;
        d->T::foo();  // B::foo (!!)
    }

That's right — `d->T::foo()` refers to the `T` that is a member of `D` — that is, the `T` that means `B` —
completely ignoring the local meaning of `T` as an alias for `A`! Contrariwise, in `c->T::foo()`, the name
`T` is looked up in the context of class `C` and is not found; therefore it is looked up a second
time in the local scope and we get `A` in that case.

This gets even more confusing when you bring templates into it.

    template<class T>
    void three(T *t) {
        t->T::foo();
    }

    template void three<C>(C*);  // calls C::foo
    template void three<D>(D*);  // calls B::foo

See, the same logic applies in this case: when `t` is of type `D`, `t->T::foo()` refers to the `T`
that is a member of `D` (that is, `B`). But when `t` is of type `C`, there is no such `T` and
therefore `T` has its local meaning as a template type parameter (that is, `C` itself).

MSVC, Clang, and ICC all handle this as shown above. GCC alone dissents: if it sees `t->T::foo` and
`t`'s type is template-dependent, it'll always use the local `T` instead
of looking it up in the scope of `t`. However, this only applies to template-dependent things!

    template<class T>
    void three(T *t) {
        t->T::foo();
    }
    template void three<D>(D*);  // GCC (wrongly) calls D::foo

    template<class T>
    void four(D *t) {
        t->T::foo();
    }
    template void four<D>(D*);  // GCC calls B::foo

[CWG issue 1089](http://cwg-issue-browser.herokuapp.com/cwg1089) deals with this kind of weirdness.

Who benefits from this confusing double-lookup rule? The behavior of function `two` above seems utterly
counterintuitive. It would be interesting to see compiler vendors emit a warning whenever this
arcane rule is triggered; my guess is that no real codebase would see that warning, and eventually
maybe C++ could get rid of the rule. (See also:
["_Contra_ implicit declarations of struct types"](/blog/2018/05/16/contra-implicit-struct-declarations/) (2018-05-16),
["Field-testing Herb Sutter’s Modest Proposal to Fix ADL"](/blog/2018/08/13/fixing-adl-field-test/) (2018-08-13).)
