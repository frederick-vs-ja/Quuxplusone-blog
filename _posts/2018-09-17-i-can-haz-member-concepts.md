---
layout: post
title: 'A use-case for member concepts and/or template concept parameters'
date: 2018-09-17 00:01:00 +0000
tags:
  concepts
  preprocessor
  rant
---

I continue, very slowly, to work on my CppCon 2018 session "Concepts As She Is Spoke."
A while back, someone on [the cpplang Slack](https://cppalliance.org/slack/)
asked whether it was possible to have either "template concept parameters" or
"member concepts" — that is, either

    template<
        template<class> concept TemplateConceptParameter
    >
    struct Foo {
        template<class T>
        void bar() requires TemplateConceptParameter<T> {
            // ...
        }
    };

    Foo<Integral> foo;
    foo.bar(1);        // OK
    foo.bar("no");     // error

or the poor man's version,

    struct integral_helper {
        template<class T>
        concept MemberConcept = Integral<T>;
    };

    template<class X>
    struct Foo {
        template<class T>
        void bar() requires X::template MemberConcept<T> {
            // ...
        }
    };

    Foo<integral_helper> foo;
    foo.bar(1);        // OK
    foo.bar("no");     // error

They were asked for a concrete use-case. Well, I've got a concrete use-case now,
albeit kind of ivory-tower.

I want to write a little snippet of C++ code to illustrate the C++2a notion of "subsumption" —
how one concept can refine another concept in a way understandable by the compiler.
The rules for subsumption are quite persnickety: sometimes the compiler understands
that concept `A` subsumes `B`, and sometimes it doesn't.

For any *given* pair of concepts `A` and `B`, we can test their relationship to each
other by creating a pair of functions, one constrained on `A` and the other constrained
on `B`, and then seeing whether that makes overload resolution happy or sad.

[So we write:](https://godbolt.org/z/wgHfIh)

    template<class T>
    concept AlwaysTrue =
        std::is_void_v<std::void_t<T>>;

    template<class T>
    concept EvenMoreTrue = AlwaysTrue<T> &&
        std::is_void_v<std::void_t<T, void>>;

    template<class Subsumes, class = int>
    struct helper : std::false_type {};

    template<class S>
    struct helper<S, decltype(S::template f<void>())>
        : std::true_type {};

    template<
        template<class> concept A
        template<class> concept B
    >
    struct Subsumes {
        template<class T> static void f()
            requires B<T> || AlwaysTrue<T>;
        template<class T> static int f()
            requires A<T> || EvenMoreTrue<T>;
        using type = typename helper<Subsumes>::type;
        static constexpr bool value = type::value;
    };

    static_assert(Subsumes<Integral, Scalar>);
    static_assert(not Subsumes<Scalar, Integral>);

This works because if `A` subsumes `B`, then `A<T> || EvenMoreTrue<T>` will subsume `B<T> || AlwaysTrue<T>`,
and incidentally both constraints will *always be satisfied*, even for `T=void`, which is how we're going
to instantiate `f()`.

So this snippet [works great](https://godbolt.org/z/wgHfIh) to verify that subsumption works the way I expect it to.
The only problem is that we can't literally write

    template<
        template<class> concept A
        template<class> concept B
    >

Instead, we have to fall back on Boost-era macro metaprogramming:

    #define DEFINE_SUBSUMES(A,B) \
    struct Subsumes##A##B { \
      template<class T> static void f() \
        requires B<T> || AlwaysTrue<T>; \
      template<class T> static int f() \
        requires A<T> || EvenMoreTrue<T>; \
      using type = typename helper<Subsumes##A##B>::type; \
      static constexpr bool value = type::value; \
    };

    DEFINE_SUBSUMES(Scalar, Integral)
    DEFINE_SUBSUMES(Integral, Scalar)
    DEFINE_SUBSUMES(NonScalar, NonIntegral)
    DEFINE_SUBSUMES(NonIntegral, NonScalar)

    static_assert(SubsumesIntegralScalar::value);
    static_assert(not SubsumesScalarIntegral::value);
    static_assert(not SubsumesNonScalarNonIntegral::value);
    static_assert(not SubsumesNonIntegralNonScalar::value);

Is this a motivating enough example to get member concepts into C++2a?
Or merely motivating enough to get subsumption *out?*
Honestly I'd accept either one as a step forward.
