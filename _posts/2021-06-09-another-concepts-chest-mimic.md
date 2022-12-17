---
layout: post
title: 'Another `requires`-clause syntax pitfall'
date: 2021-06-09 00:01:00 +0000
tags:
  compiler-diagnostics
  concepts
  implementation-divergence
  pitfalls
  cpplang-slack
excerpt: |
  In my CppCon 2018 talk ["Concepts As She Is Spoke,"](https://www.youtube.com/watch?v=CXn02MPkn8Y)
  I presented some pitfalls in `requires`-clause syntax, such as:

      template<class T>
      concept HasHash = requires (const T& t) {
          t.hash() -> std::integral;
      };

      struct X { int hash() const; };
      static_assert(!HasHash<X>);

  On the cpplang Slack today, John Eivind Helset and Eugenio Bargiacchi (inadvertently)
  showed me a new trap I hadn't seen before:
---

In my CppCon 2018 talk ["Concepts As She Is Spoke,"](https://www.youtube.com/watch?v=CXn02MPkn8Y)
I presented some pitfalls in `requires`-clause syntax, such as
([Godbolt](https://godbolt.org/z/bz6rE9PMq)):

    template<class T>
    concept HasHash = requires (const T& t) {
        t.hash() -> std::integral;
    };

    struct X { int hash() const; };
    static_assert(!HasHash<X>);

On the cpplang Slack today, John Eivind Helset and Eugenio Bargiacchi (inadvertently)
showed me a new trap I hadn't seen before ([Godbolt](https://godbolt.org/z/7dbdanr3n)):

    template<class T>
    concept HasAButNotB = requires (T t) {
        t.a();
        !requires { t.b(); };
    };

    struct S1 {};
    struct S2 { void a(); };
    struct S3 { void b(); };
    struct S4 { void a(); void b(); };

    static_assert(!HasAButNotB<S1>);  // as expected
    static_assert(HasAButNotB<S2>);   // as expected
    static_assert(!HasAButNotB<S3>);  // as expected
    static_assert(HasAButNotB<S4>);   // ...oops!

Interestingly, most compilers not only warn but _error out_ on

    template<class T>
    concept HasX = requires (T t) {
        requires { t.x(); };
    };

but as soon as you put `operator!` in front of it, every compiler
accepts it without complaint.

As usual, the problem is that we're checking for the well-formedness
of the expression `!requires { t.b(); }`, not the truth value of that
expression.

Each different vendor diagnoses a slightly different subset of possible
user errors in this area:

|:------------------------|:----------------:|:---:|:---:|:-----:|:----:|
| _requirement_           | Did you mean it? | Is it valid C++20? | GCC | Clang | MSVC |
|:------------------------|:----------------:|:---:|:---:|:-----:|:----:|
| `t.b();`                | yes              | yes | OK  | OK | OK |
| `t.b() -> X;`           | no               | yes | OK  | OK | OK |
| `t.b() -> std::X;`      | no               | yes | syntax error | OK | warning C5207 at declaration |
| `requires t.b();`       | probably not     | yes | hard error if `t.b()` exists | OK | hard error if `t.b()` exists |
| `requires { t.b(); };`  | no               | no? | syntax error | `-Wrequires-expression` at declaration | syntax error |
| `!requires t.b();`      | no               | no  | syntax error | syntax error | syntax error |
| `!requires { t.b(); };` | no               | yes | OK | OK | OK |
|:------------------------|:----------------:|:---:|:---:|:-----:|:----:|
{:.smaller}

----

See also:

* ["Two musings on the design of compiler warnings"](/blog/2020/09/02/wparentheses/) (2020-09-02)
