---
layout: post
title: "Name lookup in multiple base classes"
date: 2024-04-17 00:01:00 +0000
tags:
  c++-learner-track
  classical-polymorphism
  customization-points
  name-lookup
excerpt: |
  Several times people have asked me, "Why does overload resolution not work if the
  overload set spans two base classes?" That is:

      struct Plant {
        void f(int);
      };
      struct Fungus {
        void f(int, int);
      };
      struct Lichen : Plant, Fungus {
        void g() {
          f(1);     // error, lookup fails
          f(1, 2);  // error, lookup fails
        }
      };

  This is because [[class.member.lookup]](https://timsong-cpp.github.io/cppwp/n4868/class.member.lookup#6.2)
  says, essentially, that if we don't find a declaration of the name `f` in `Lichen`'s
  scope then we should look into its base classes; and if we find declarations of `f`
  in _more than one_ base class, we should consider this an unresolvable ambiguity
  and fail.
---

Several times people have asked me, "Why does overload resolution not work if the
overload set spans two base classes?" That is:

    struct Plant {
      void f(int);
    };
    struct Fungus {
      void f(int, int);
    };
    struct Lichen : Plant, Fungus {
      void g() {
        f(1);     // error, lookup fails
        f(1, 2);  // error, lookup fails
      }
    };

This is because [[class.member.lookup]](https://timsong-cpp.github.io/cppwp/n4868/class.member.lookup#6.2)
says, essentially, that if we don't find a declaration of the name `f` in `Lichen`'s
scope then we should look into its base classes; and if we find declarations of `f`
in _more than one_ base class, we should consider this an unresolvable ambiguity
and fail.

In my training materials I always motivate this behavior by using functions that take strings:

    struct OldSchoolPlant {
      void f(const char *);
      void f(const std::string&);
    };

`OldSchoolPlant` uses the pre-C++17 idiom for efficiently taking a string parameter:
If the caller passes a `std::string`, overload resolution will select `f(const std::string&)`,
which efficiently takes only a reference to their existing `string` object. If the caller passes
something convertible to `string`, we'll end up constructing a temporary `string` object and
taking a reference to that. But, if the caller passes a string literal, overload resolution
will prefer `f(const char*)`, which avoids constructing a temporary `string` at all.

In C++17 and later, we can use `string_view` instead:

    struct NewSchoolFungus {
      void f(std::string_view);
    };

Now, no matter what kind of argument the caller passes, we never make a temporary `string`.
We just convert the argument to `string_view` (a trivially copyable, reference-semantic,
[parameter-only](/blog/2018/03/27/string-view-is-a-borrow-type/) type). We no longer need
two overloads.

Suppose `MixedLichen` inherits from both `OldSchoolPlant` and `NewSchoolFungus`
([Godbolt](https://godbolt.org/z/EYrj7W7zT)):

    struct MixedLichen : OldSchoolPlant, NewSchoolFungus {
      void g(const std::string& s) {
        f(s);
        f("literal");
      }
    };

Here we don't want to mash together the overload sets of
`OldSchoolPlant::f` and `NewSchoolFungus::f`. They were designed individually, using
totally different paradigms (pre-C++17 and post-C++17). They weren't designed to play together!
To say it another way: It would be bad if the well-formedness of the above calls to `f`
depended on such "internal details" of `OldSchoolPlant` and `NewSchoolFungus` as
whether they were written in pre-C++17 or post-C++17 style.

To say it yet another way: _The unit of C++ API design is the overload set._
(Titus Winters, ["Modern C++ API Design,"](https://www.youtube.com/watch?v=xTdeZ4MxbKo) CppCon 2018.)
When we update something like `OldSchoolPlant::f` from pre-C++17 to post-C++17
style, we're refactoring a _whole overload set,_ not just a single function. The only sensible
way to refactor an API is to refactor a whole overload set at a time. _If_ C++ were
habitually to mash multiple widely dispersed overload sets into one, that would,
by definition, increase coupling and hamper refactoring. Which is bad.

> This is why library maintainers tend to hate ADL: indispensable as it may be,
> its entire purpose is to mash together widely dispersed overload sets.
> See ["What is ADL?"](/blog/2019/04/26/what-is-adl/) (2019-04-26);
> ["How `hana::type<T>` disables ADL"](/blog/2019/04/09/adl-insanity-round-2/) (2019-04-09).

Good news — C++'s name lookup rules _do not_ mash together the overload sets of
`OldSchoolPlant::f` and `NewSchoolFungus::f`! When we do name lookup in a class,
if we find member declarations in _more than one_ of its base classes, we consider
it an unresolvable ambiguity and fail.

This ambiguity and failure happens during name lookup, which means it happens _before_
any part of overload resolution. In particular, going back to our original example, the lookup
for `f` in `Lichen` will find both `Plant::f(int)` and `Fungus::f(int, int)`. These two functions
have different arities (numbers of arguments) — but arity plays a role only in overload resolution,
not in name lookup; and since lookup fails, we never _get_ to the overload resolution step.

> During name lookup, by definition, we don't yet know what kind of entity the name refers to;
> so we don't yet know if it even _is_ a function! So its "number of arguments" can't form part of our
> calculation. Again, ADL breaks the rules by looking at the argument list; but ADL happens only
> after ordinary lookup, and only if ordinary lookup did find a non-member function or function template,
> so that we can be sure we're dealing with a function and its arguments.
> See ["To enable ADL, add a using-declaration"](/blog/2022/04/30/poison-pill-redux/) (2022-04-30).

In short, mashing together unrelated overload sets is usually a bad thing; and therefore the
rules of C++ are _generally_ designed not to do it.

## What if I _want_ to mash together my bases' overload sets?

You have at least two options. One is to use member `using`-declarations to bring each base's
name into your own class's scope ([Godbolt](https://godbolt.org/z/jjxG31Wfv)):

    struct Plant {
      void f(int);
    };
    struct Fungus {
      void f(int, int);
    };
    struct Lichen : Plant, Fungus {
      using Plant::f;
      using Fungus::f;
      void g() {
        f(1);     // OK, finds Plant::f(int) in Lichen's scope
        f(1, 2);  // OK, finds Fungus::f(int, int) in Lichen's scope
      }
    };

Now name lookup doesn't look into the base classes at all, because it finds declarations of `f`
right here inside `Lichen`. The original declarations in `Plant` and `Fungus` are _shadowed_
by our manual `using`-declarations.

If you're doing this a lot, such that [it hurts](https://www.youtube.com/watch?v=ri3aL8At44I&t=1m25s),
you _could_ hide the repetitive `using`-declarations behind a helper template ([Godbolt](https://godbolt.org/z/sTscWY15n)):

    template<class... Ts>
    struct FHelper : Ts... {
      using Ts::f...;
    };

    struct Lichen : FHelper<Plant, Fungus> {
      void g() {
        f(1);     // OK, finds Plant::f(int) in FHelper's scope
        f(1, 2);  // OK, finds Fungus::f(int, int) in FHelper's scope
      }
    };

A crazier option (which I mention because it's interesting; it's not a good idea)
is to use ADL — the core-language feature whose entire purpose is to mash together
widely dispersed overload sets. ([Godbolt.](https://godbolt.org/z/s5Wovaj6T))

    namespace detail { void adl_f() = delete; }
    struct Plant {
      friend void adl_f(Plant*, int);
    };
    struct Fungus {
      friend void adl_f(Fungus*, int, int);
    };
    struct Lichen : Plant, Fungus {
      void g() {
        using detail::adl_f;
        adl_f(this, 1);     // OK, ADL finds Plant's adl_f
        adl_f(this, 1, 2);  // OK, ADL finds Fungus's adl_f
      }
    };

This last was basically the shape of the formerly proposed "`tag_invoke`," for which see Barry Revzin's
["Why `tag_invoke` is not the solution I want"](https://brevzin.github.io/c++/2020/12/01/tag-invoke/) (December 2020).
After that blog post, `tag_invoke` was merged into the only paper that cared about it, P2300 "`std::execution`"
(a.k.a. Senders and Receivers), and has at last been excised completely from
[P2300R9](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r9.html#design-dispatch) (April 2024).
