---
layout: post
title: "A deduction guide for `Foo(Ts..., Ts...)`"
date: 2023-05-26 00:01:00 +0000
tags:
  class-template-argument-deduction
  cpplang-slack
  implementation-divergence
  metaprogramming
  variadic-templates
excerpt: |
  On [the cpplang Slack](https://cppalliance.org/slack/), Håkon B. Grundnes
  posed a metaprogramming problem: Write a
  [CTAD](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#ctad)
  deduction guide that works for this class template.

      template<class... Ts>
      class Foo {
      public:
        explicit Foo(Ts... as, Ts... bs) :
          a_(static_cast<Ts&&>(as)...),
          b_(static_cast<Ts&&>(bs)...) {}

      private:
        std::tuple<Ts...> a_;
        std::tuple<Ts...> b_;
      };
---

On [the cpplang Slack](https://cppalliance.org/slack/), Håkon B. Grundnes
posed a metaprogramming problem: Write a
[CTAD](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#ctad)
deduction guide that works for this class template.

    template<class... Ts>
    class Foo {
    public:
      explicit Foo(Ts... as, Ts... bs) :
        a_(static_cast<Ts&&>(as)...),
        b_(static_cast<Ts&&>(bs)...) {}

    private:
      std::tuple<Ts...> a_;
      std::tuple<Ts...> b_;
    };

That is, we want [all these cases](https://godbolt.org/z/qhE34bjrP) to compile:

    Foo f0 = Foo(); // deduces Foo<>
    Foo f1 = Foo(1, 2); // deduces Foo<int>
    Foo f2 = Foo(1, 'a', 2, 'b'); // deduces Foo<int, char>
    Foo f3 = Foo("sun", "moon"); // deduces Foo<const char*>

Here's my (partial) solution.

----

Initially, let's ignore the CTAD part of the problem: assume we'll
use a `make_pair`–like factory function instead. This lets us focus
on the algorithmic problem.

The algorithmic problem is to take a single parameter pack of the form
`Ts..., Ts...` and split it up to find `Ts...` alone. We can divide that problem
into two subtasks: First, given a pack of length `n`, yield its first
`n/2` elements; and second, verify that the second `n/2` elements in the original
pack are the same as the first `n/2` elements — that is, that the whole pack
is equal to the half-pack repeated.

So our first draft of a solution looks something like this:

    template<class... TsTs>
    auto make_foo(TsTs... tsts) {  // rough-draft version
      using FooTs = typename foo_of_first_half<TsTs...>::type;
      if constexpr (is_doubled<FooTs, Foo<TsTs...>>::value) {
        return FooTs(tsts...);
      } else {
        // Fail. Ideally make_foo would SFINAE away in this case.
      }
    }

The easiest way to write `foo_of_first_half`, I think, is to use `tuple_element_t`:

    template<class... TsTs>
    struct foo_of_first_half {
      template<size_t I>
      using IthElement = std::tuple_element_t<I, std::tuple<TsTs...>>;

      template<size_t... Is>
      static auto f(std::index_sequence<Is...>)
        -> Foo<IthElement<Is>...>;

      static constexpr size_t N = sizeof...(TsTs) / 2;
      using type = decltype(f(std::make_index_sequence<N>()));
    };

The easiest way to write `is_doubled` is certainly to pattern-match using
a partial specialization. Just as we can implement `std::is_same` like this:

    template<class, class>
    struct is_same : false_type {};

    template<class T>
    struct is_same<T, T> : true_type {};

we can write our `is_doubled` like this:

    template<class, class>
    struct is_doubled : false_type {};

    template<class... Ts>
    struct is_doubled<Foo<Ts...>, Foo<Ts..., Ts...>> : true_type {};

----

Putting it all together, and adding the missing SFINAE, we get this.

    template<class... TsTs>
    struct foo_of_first_half {
      template<size_t I>
      using IthElement = std::tuple_element_t<I, std::tuple<TsTs...>>;

      template<size_t... Is>
      static auto f(std::index_sequence<Is...>)
        -> Foo<IthElement<Is>...>;

      static constexpr size_t N = sizeof...(TsTs) / 2;
      using type = decltype(f(std::make_index_sequence<N>()));
    };

    template<class, class>
    struct is_doubled : std::false_type {};

    template<class... Ts>
    struct is_doubled<Foo<Ts...>, Foo<Ts..., Ts...>> : std::true_type {};

    template<class... TsTs, class FooTs = typename foo_of_first_half<TsTs...>::type>
      requires is_doubled<FooTs, Foo<TsTs...>>::value
    FooTs make_foo(TsTs... tsts) {
      return FooTs(tsts...);
    }

This version fails to perfect-forward the arguments to `Foo`'s constructor,
which means we're making a lot more copies than we'd expect. We want to take `TsTs&&...`,
not `TsTs...` — but, at the same time, we want `make_foo("abc", "def")` to yield
`Foo<const char*>`, not `Foo<const char(&)[4]>`! We can fix that in either of two ways:
either by applying `decay_t` ([Godbolt](https://godbolt.org/z/zz5fcKajb))—

    template<class... TsTs, class FooTs = typename foo_of_first_half<std::decay_t<TsTs>...>::type>
      requires is_doubled<FooTs, Foo<std::decay_t<TsTs>...>>::value
    FooTs make_foo(TsTs&&... tsts) {
      return FooTs(std::forward<TsTs>(tsts)...);
    }

—or by deducing the real return type based on the result type of a never-called helper function,
similar to how CTAD deduction guides actually work today ([Godbolt](https://godbolt.org/z/zTrcxnbbP))—

    template<class... TsTs, class FooTs = typename foo_of_first_half<TsTs...>::type>
      requires is_doubled<FooTs, Foo<TsTs...>>::value
    FooTs make_foo_guide(TsTs... tsts);

    template<class... TsTs, class FooTs = decltype(make_foo_guide(std::declval<TsTs>()...))>
    FooTs make_foo(TsTs&&... tsts) {
      return FooTs(std::forward<TsTs>(tsts)...);
    }


## But what about CTAD?

The solution above allows us to write:

    Foo f2 = make_foo(1, 'a', 2, 'b');

But we originally asked for:

    Foo f2 = Foo(1, 'a', 2, 'b');

In an ideal world, we'd be able to take the metaprogramming we just did, and stick it
on the right-hand side of a deduction guide:

    template<class... TsTs>
    Foo(TsTs&&...) -> decltype(make_foo(std::declval<TsTs>()...));

But no vendor supports this syntax! Clang says:

    error: deduced type 'decltype(make_foo(std::declval<TsTs>()...))'
    of deduction guide is not written as a specialization
    of template 'Foo'
      Foo(TsTs&&...) -> decltype(make_foo(std::declval<TsTs>()...));
                        ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Standard C++ requires that the thing on the right-hand side of the `->` be
pretty much exactly the sequence of characters `Foo<` followed by some template
arguments; it's not allowed to use a helper function, or a type alias.
The wording in [[temp.deduct.guide]](https://eel.is/c++draft/temp.deduct.guide#3.sentence-3) is
draconian:

> *deduction-guide*:  
> &nbsp; &nbsp; *explicit-specifier<sub>opt</sub> template-name* `(` *parameter-declaration-clause* `)` `->` *simple-template-id* `;`
>
> The *simple-template-id* shall name a class template specialization.
> The *template-name* shall be the same *identifier* as the *template-name* of the *simple-template-id*.

There's a little implementation divergence here; for example,
where the Standard requires a [_simple-template-id_](https://eel.is/c++draft/temp.names#nt:simple-template-id),
all vendors in practice allow a [_qualified-id_](https://eel.is/c++draft/expr.prim.id.qual#nt:qualified-id).
So one can in practice write:

    struct Outer {
      template<class> struct Inner;
      Inner(int) -> Outer::Inner<int>; // all three accept
    };

GCC and MSVC stretch as far as permitting a type alias if the alias expands to a
specialization directly:

    template<class> struct S;
    template<class T> using Alias = S<T>;
    template<class T> S(T) -> Alias<T>; // GCC+MSVC accept; Clang rejects

Or if the alias expands to something non-dependent:

    template<class> struct S;
    template<class T> using Alias = std::type_identity_t<S<T>>;
    template<class T> S(T) -> Alias<int>; // GCC+MSVC accept; Clang rejects

But no vendor stretches so far as to permit arbitrary computations on types:

    template<class> struct S;
    template<class T> using Alias = std::type_identity_t<S<T>>;
    template<class T> S(T) -> Alias<T>; // all three reject

Under these restrictions, I believe it's impossible to implement a CTAD deduction guide
with the same behavior as our `make_foo`. But I would be very favorably disposed toward
a proposal that removed these unnecessary restrictions!

----

For more on CTAD's inferiority to factory functions in practice, see also:

* ["Replacing `std::lock_guard` with a factory function"](/blog/2022/12/14/my-lock-guard/) (2022-12-14)
