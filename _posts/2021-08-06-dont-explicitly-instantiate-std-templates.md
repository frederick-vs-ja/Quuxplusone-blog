---
layout: post
title: "Don't explicitly instantiate `std` templates"
date: 2021-08-06 00:01:00 +0000
tags:
  library-design
  pitfalls
  sd-8-adjacent
---

I hope by now everybody knows not to forward-declare standard library types.
For example, this code ([Godbolt](https://godbolt.org/z/938oqY4f7)) is wrong:

    namespace std {
        template<class T> class vector;
    }
    void foo(const std::vector<int> *);

It's wrong for at least two reasons:

* It's easy to forget that `vector` is a template of two parameters, not just one. That's easy to fix.

* `namespace std` can contain arbitrarily many inline namespaces. That's impossible to fix
    (without special knowledge of your library vendor's implementation).

So, we know not to forward-declare standard library types.

> To help enforce this, I repeat the mantra "Never reopen `namespace std`."
> Forward declarations are just one of several ill-advised practices prevented
> by refusing to reopen `namespace std`. For example, it also prevents people
> from trying to add their own overloads of `std::swap` — a sin popular in
> previous decades but now thankfully receding. The mantra doesn't interfere
> with providing one's own `std::hash` specialization: one can write
> `template<> struct std::hash<MyType> {...};` without reopening the whole
> namespace.

This week I learned of a related pitfall — one that doesn't involve reopening
`namespace std` — thanks to [an issue raised on libc++](https://reviews.llvm.org/D62259#2922952).
Consider the following translation unit ([Godbolt](https://godbolt.org/z/jYv65EnvK)):

    #include <atomic>
    template class std::atomic<void*>;

That's an [_explicit instantiation definition_](https://eel.is/c++draft/temp.explicit#2):
very rare in normal code. (And for good reason, as we'll see!)

This translation unit is invalid; in fact, GNU libstdc++ makes it a hard error.
Having an object of type `atomic<void*>` is fine; the problem here
is with the semantics of explicit instantiation. Ordinary _implicit_ instantiation on
class templates is fundamentally lazy: the compiler is guaranteed to instantiate
only the bits of the class that it actually needs. So for example this is perfectly legal:

    template<class T>
    struct S {
        static_assert(sizeof(T) < 0);
    };

    S<int> *p = nullptr;  // instantiates only the declaration

but this is not:

    S<int> s;  // instantiates the definition: boom!

And this is perfectly legal:

    template<class T>
    struct A {
        void f() { }
        void g() { T& t = 42; }
    };

    int main() {
        A<int> a;
        a.f();  // instantiates only f: OK
    }

but this is not:

        a.g();  // instantiates g: boom!

But an explicit instantiation definition isn't lazy.
It eagerly instantiates _every_ member of the class. (At least,
every member it can. This [excludes](https://eel.is/c++draft/temp.spec#temp.explicit-10)
members that are themselves templates, as well as members whose definitions aren't
available at the point of instantiation and/or whose constraints aren't satisfied.) Thus:

    template struct A<int>;  // instantiates f and g: boom!

Standard library templates tend to rely on the laziness of implicit instantiation,
and thus may break horribly if you instantiate all the members all at once.

> This is actually just one concrete case of the general rule that you should
> never tell a C++ implementation "Forget discretion and just give me everything you've got!"
> because "literally everything" will invariably include some dangerous or foolish things too.
> For other examples, see ["Don't put `-Weverything` in your build flags"](/blog/2018/12/06/dont-use-weverything/) (2018-12-06)
> and ["Don't inherit from standard types"](/blog/2018/12/11/dont-inherit-from-std-types/) (2018-12-11).

In the specific case of libstdc++'s `atomic<void*>`, it's structured very roughly like
this:

    template<class A>
    concept incrementable = requires (A a) { ++a; };

    template<class T>
    class AtomicPtr1 {
        std::atomic<char*> p_;
    public:
        T *operator++() requires incrementable<T*> {
            return static_cast<T*>(p_.fetch_add(sizeof(T)));
        }
    };
    static_assert(incrementable<AtomicPtr1<int>>);
    static_assert(!incrementable<AtomicPtr1<void>>);
    template class AtomicPtr1<int>;   // OK
    template class AtomicPtr1<void>;  // OK

(libstdc++ disables `operator++` with a conditional base class,
not a C++20 `requires` clause; this version is still roughly equivalent.)
Then, libstdc++ pulls out the `sizeof` computation into a private static
helper function. This simple refactoring gives explicit instantiation
an unconstrained non-template member to latch onto, and boom goes the dynamite
([Godbolt](https://godbolt.org/z/WoKbEcocP)):

    template<class T>
    class AtomicPtr2 {
        std::atomic<char*> p_;
        static int private_size() { return sizeof(T); }  // NEW
    public:
        T *operator++() requires incrementable<T*> {
            return static_cast<T*>(p_.fetch_add(private_size()));
        }
    };
    static_assert(incrementable<AtomicPtr2<int>>);
    static_assert(!incrementable<AtomicPtr2<void>>);

    template class AtomicPtr2<int>;   // OK
    template class AtomicPtr2<void>;  // boom

Version with the computation inlined: OK. Version with the computation factored
out into a helper: Error! This is just one example of the
kinds of things that can go wrong when you explicitly instantiate
templates from the standard library — or, really, from _any_ library
whose evolution you don't control.

> Don't explicitly instantiate `std` templates.


## Language-lawyer footnote

Section [[namespace.std]/5](https://eel.is/c++draft/library#namespace.std-5)
of the current paper standard claims that it's actually okay to explicitly instantiate
"a class template defined in the standard library ... if the declaration depends on
the name of at least one program-defined type ..." That is, according to the paper
standard it should be okay to do things like

    struct Widget {};
    template class std::atomic<Widget(*)()>;
    template class std::shared_ptr<Widget[]>;

However, in practice it's not okay, as outlined above; and it will likely never
be okay, because it blows up for strictly technological reasons. You can't
outlaw "pulling things out into private helper functions"!


## Practical footnote

Clang, but not GCC, supports an attribute named `exclude_from_explicit_instantiation`
that makes helper functions "privater than private." So, on Clang at least,
we could fix `AtomicPtr2` like this ([Godbolt](https://godbolt.org/z/WfYxhb1TP)):

    template<class T>
    class AtomicPtr3 {
        std::atomic<char*> p_;
        __attribute__((exclude_from_explicit_instantiation))
        static int private_size() { return sizeof(T); }
    public:
        T *operator++() requires incrementable<T*> {
            return static_cast<T*>(p_.fetch_add(private_size()));
        }
    };
    static_assert(incrementable<AtomicPtr3<int>>);
    static_assert(!incrementable<AtomicPtr3<void>>);

    template class AtomicPtr3<int>;   // OK
    template class AtomicPtr3<void>;  // OK now!

Now the explicit instantiation of `AtomicPtr3<int>` no longer instantiates
`AtomicPtr3<int>::private_size`. And the explicit instantiation
of `AtomicPtr3<void>` no longer instantiates `AtomicPtr3<void>::private_size`,
which means it doesn't go boom anymore. Using this attribute, the
class designer can actually craft the set of members they want to
be explicitly instantiated versus the set of members they want to exclude.

> Earlier I compared explicit instantiations to Clang's `-Weverything`.
> This attribute allows the class designer to craft a selection
> analogous to `-Wall`: just a handful of members that the designer
> thinks you might actually want.

This attribute was actually
invented in late 2018 for the benefit of libc++ in some obscure
shared-object situation (Louis Dionne documented it well:
[[1]](https://lists.llvm.org/pipermail/cfe-dev/2018-August/059024.html),
[[2]](https://reviews.llvm.org/D51789),
[[3]](https://reviews.llvm.org/D52405)).
I don't think ordinary library programmers should ever apply
`__attribute__((exclude_from_explicit_instantiation))`,
but it's interesting
to know it exists.
