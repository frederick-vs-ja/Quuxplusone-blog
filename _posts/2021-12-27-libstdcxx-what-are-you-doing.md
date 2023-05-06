---
layout: post
title: "Completeness preconditions considered harmful"
date: 2021-12-27 00:01:00 +0000
tags:
  concepts
  cpplang-slack
  implementation-divergence
  library-design
  metaprogramming
  pitfalls
  ranges
excerpt: |
  Here's a C++ riddle for you: When does `std::invoke` not invoke?

      struct Incomplete;
      Incomplete&& give();
      void take(Incomplete&&);

      void okay() {
          take(give());  // OK, valid C++
      }

      void bad() {
          std::invoke(take, give());  // UB
      }

  Given this code ([Godbolt](https://godbolt.org/z/4rsf98rqM)),
  libc++ and Microsoft accept the call to `invoke`, but libstdc++
  rejects with a spew of errors:

      type_traits:3027:7: error: static_assert failed due to requirement
          'std::__is_complete_or_unbounded(std::__type_identity<Incomplete>{})'
          "each argument type must be a complete class or an unbounded array"
        static_assert((std::__is_complete_or_unbounded(
        ^              ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
---

Here's a C++ riddle for you: When does `std::invoke` not invoke?

    struct Incomplete;
    Incomplete&& give();
    void take(Incomplete&&);

    void okay() {
        take(give());  // OK, valid C++
    }

    void bad() {
        std::invoke(take, give());  // UB
    }

Given this code ([Godbolt](https://godbolt.org/z/4rsf98rqM)),
libc++ and Microsoft accept the call to `invoke`, but libstdc++
rejects with a spew of errors:

    type_traits:3027:7: error: static_assert failed due to requirement
        'std::__is_complete_or_unbounded(std::__type_identity<Incomplete>{})'
        "each argument type must be a complete class or an unbounded array"
      static_assert((std::__is_complete_or_unbounded(
      ^              ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    type_traits:3034:5: note: in instantiation of template class
        'std::invoke_result<void (&)(Incomplete &&), Incomplete>' requested here
      using invoke_result_t = typename invoke_result<_Fn, _Args...>::type;
      ^
    functional:106:33: note: in instantiation of template type alias
        'invoke_result_t' requested here
      inline _GLIBCXX20_CONSTEXPR invoke_result_t<_Callable, _Args...>
                                  ^
    note: while substituting deduced template arguments into function template
        'invoke' [with _Callable = void (&)(Incomplete &&), _Args = <Incomplete>]
      std::invoke(take, give());  // UB
      ^
    error: no matching function for call to 'invoke'
      std::invoke(take, give());  // UB
      ^~~~~~~~~~~
    functional:107:5: note: candidate template ignored: substitution failure
        [with _Callable = void (&)(Incomplete &&), _Args = <Incomplete>]
      invoke(_Callable&& __fn, _Args&&... __args)
      ^

You see, [`std::invoke`](https://eel.is/c++draft/func.invoke#1)
takes a list of arguments `(F&&, Args&&...)`, and is constrained
to participate in overload resolution only when `std::is_invocable<F, Args...>::value`
is `true`. (Notice that it strips the ref-qualifiers from `F&&` and `Args&&`!)
Meanwhile, [`is_invocable`](https://eel.is/c++draft/meta.rel#tab:meta.rel-row-8-column-3-sentence-1)
has undefined behavior whenever any of its arguments are incomplete types.

When `std::invoke` is instantiated with `F = void(&)(Incomplete&&), Args = Incomplete&&`,
it strips the ref-qualifiers and asks whether `std::is_invocable<void(&)(Incomplete&&), Incomplete>`.

`Incomplete&&` (being a reference type) is complete, but
`Incomplete` (the corresponding object type) is incomplete.
So when `std::invoke` asks whether `std::is_invocable<void(&)(Incomplete&&), Incomplete>`,
that violates `is_invocable`'s preconditions on and renders the program
ill-formed, no diagnostic required.

The above program is IFNDR according to the paper standard. libc++ and Microsoft
both do the obvious thing, which is to let you compile it anyway with the obvious
behavior. As of this writing, libstdc++ rejects the program with a hard error.

I suggest three fixes to the paper standard here:


## 1. Fix `invoke` specifically

`invoke(F&&, Args&&...)` should be constrained on `is_invocable<F&&, Args&&...>`,
not on `is_invocable<F, Args...>`. This would immediately eliminate one source of
undefined behavior in the library, and would be very cheap to implement.

UPDATE, 2021-12-28: Conversation with Peter Dimov reveals that we'd also have to
change the return type of `invoke` from `invoke_result_t<F, Args...>` to
`invoke_result_t<F&&, Args&&...>`, because `invoke_result`
[has the same completeness precondition](https://eel.is/c++draft/meta.trans.other#tab:meta.trans.other-row-13-column-2-sentence-6)
as `is_invocable`.


## 2. Relax superfluous completeness constraints in [meta]

Basically every time a type-trait says "`X` shall be a complete type," that's a
source of undefined behavior and implementation divergence. `is_invocable` may
be the rare extreme case where completeness is obviously and wholly irrelevant.
For several other type-traits, such as `is_polymorphic`, the paper standard uses the formula

> If `T` is a non-union class type, `T` shall be a complete type.

This formula could be safely applied to several other type-traits, such as `is_aggregate`
and `is_destructible`.


## 3. Remove all completeness constraints in [meta]

Walter Brown's [P1285 "Improving Completeness Requirements for Type Traits"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1285r0.pdf)
(October 2018) added the following paragraph to [[meta.rqmts]](https://eel.is/c++draft/meta.rqmts#5):

> Unless otherwise specified, an incomplete type may be used to instantiate a
> template specified in [meta]. The behavior of a program is undefined if:
>
> - an instantiation of a template specified in [meta] directly or indirectly
>     depends on an incompletely-defined object type `T`, and
>
> - that instantiation could yield a different result were `T` hypothetically completed.

Because of this blanket wording, it is already undefined behavior for a program
to do anything like this:

    struct Incomplete;
    static_assert(!std::is_invocable_v<Incomplete&, int>); // UB
    struct Incomplete {
        void operator()(int);
    };
    static_assert(std::is_invocable_v<Incomplete&, int>);

This is basically analogous to the [One Definition Rule](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#odr):
you can't have `is_invocable_v<Incomplete&, int>` mean `false` in one part of
the program and `true` in a different part.

So the language doesn't need any _additional_ wording to guard against
incomplete types in type-traits. The blanket wording in [meta.rqmts]
already makes the problematic cases undefined. Making _non_-problematic
cases _also_ undefined is... problematic.

[P1285](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1285r0.pdf) was
also mildly concerned with the fact that as the standard stands now,

    static_assert(!std::is_assignable_v<void, Incomplete&&>); // OK
    static_assert(!std::is_assignable_v<void, Incomplete>);  // IFNDR

even though both type-traits are simply asking whether the expression
`declval<void>() = declval<Incomplete>()` would be well-formed. As with
`is_invocable`, it's weird that there are two ways to spell the same trait,
with the only difference being that one way is IFNDR and the other isn't.
However, P1285 was unable to come up with a solution for this one; it remains
[LWG3099](https://cplusplus.github.io/LWG/issue3099).

My modest proposal here is that the standard should just eliminate _all_
completeness requirements in [meta]. Let P1285's blanket wording be the
only mention of completeness in that entire section. Then it'll be safe
to use these type-traits in constraints — whereas right now it's
spectacularly unsafe (at least on libstdc++).
