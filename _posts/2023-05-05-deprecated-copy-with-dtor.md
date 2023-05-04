---
layout: post
title: "Polymorphic types and `-Wdeprecated-copy-dtor`"
date: 2023-05-05 00:01:00 +0000
tags:
  c++-style
  classical-polymorphism
  compiler-diagnostics
  cpplang-slack
  llvm
excerpt: |
  I'm a huge proponent of clearly distinguishing value-semantic types (like
  `string` and `unique_ptr<int>`) from classically polymorphic, inheritance-based
  types (like `memory_resource` and `exception`).

  - Never inherit from a value-semantic type (and
      [especially not](/blog/2018/12/11/dont-inherit-from-std-types/)
      from `std` types).

  - Never copy, move, assign, or swap a polymorphic type.

  This means that polymorphic types should usually `=delete` all their
  [SMFs](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#smf), like this:
---

I'm a huge proponent of clearly distinguishing value-semantic types (like
`string` and `unique_ptr<int>`) from classically polymorphic, inheritance-based
types (like `memory_resource` and `exception`).

- Never inherit from a value-semantic type (and
    [especially not](/blog/2018/12/11/dont-inherit-from-std-types/)
    from `std` types).

- Never copy, move, assign, or swap a polymorphic type.

This means that polymorphic types should usually `=delete` all their
[SMFs](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#smf), like this:

    struct Base1 {
        explicit Base1(some, args);

        Base1(Base1&&) = delete;
        Base1(const Base1&) = delete;
        Base1& operator=(Base1&&) = delete;
        Base1& operator=(const Base1&) = delete;

        virtual ~Base1() = default;
    };

But I admit that in my own codebases I often just do this:

    struct Base2 {
        explicit Base2(some, args);
        virtual ~Base2() = default;
    };

The compiler will still quietly generate a defaulted copy constructor
and a defaulted copy-assignment operator for `Base2`; but that happens only for historical
reasons (namely, that we didn't really understand the importance of the
[Rule of Three](https://en.cppreference.com/w/cpp/language/rule_of_three)
until after C++98 was standardized), and they won't get codegenned unless someone actually
uses them, which I won't. In fact, C++11 [deprecated](https://eel.is/c++draft/depr.impldec)
the implicit generating of copy operations for `Base2`; one day `Base2` might become
the really and truly recommended way to write a polymorphic class type.

But as of this writing, `Base2` implicitly provides copy operations, which technically
permits anyone — say, a new-hire coworker of yours — to accidentally (or intentionally)
write code that copies or swaps something derived from `Base2`. In codebases I control,
I don't mind that that's technically possible; but to anyone who asks, I'll recommend
the more explicit `Base1` style.

A couple weeks ago, I learned another advantage of `Base1` over `Base2`.


## `Base2` can trigger deprecation warnings

Clang diagnoses deprecated features under the (off-by-default) `-Wdeprecated` flag.
If you turn it on, you'll see a warning every time you use the deprecated
implicitly-defaulted copy operations of `Base2`. ([Godbolt.](https://godbolt.org/z/cc3hW3dTc))

    void test() {
        Base2 b;
        auto copy = b;
    }

    warning: definition of implicit copy constructor for 'Base2' is deprecated
    because it has a user-declared destructor [-Wdeprecated-copy-with-dtor]
        virtual ~Base2() = default;
                ^
    note: in implicit copy constructor for 'Base2' first required here
        auto copy = b;
                    ^

This is good in general, but there are a couple ways to trigger this warning
that can be a little annoying.


### Intentional cloning

First, here's the one situation I know where one might copy a polymorphic type
intentionally: the "clone" pattern.

    struct Animal {
        explicit Animal() = default;
        virtual std::unique_ptr<Animal> clone() const = 0;
        virtual std::string noise() const = 0;
        virtual ~Animal() = default;
    };

    struct Cat : public Animal {
        std::unique_ptr<Animal> clone() const override {
            return std::make_unique<Cat>(*this);
        }
        std::string noise() const override {
            return "meow";
        }
    };

    int main() {
        std::unique_ptr<Animal> a = std::make_unique<Cat>();
        auto b = a->clone();
        assert(b->noise() == "meow");
    }

`Cat::clone` calls the implicitly defaulted `Cat(const Cat&)`: no problem.
Then `Cat(const Cat&)` tries to copy its `Animal` base-class subobject via the
`Animal(const Animal&)` that was implicitly generated despite `Animal`'s user-declared
destructor: _that's_ deprecated.

The solution here is either to completely refactor your codebase to eliminate
the "clone" pattern (my preference, if you can swing it); or else to mildly edit it
so that the code clearly reflects your intent. You intend `Animal` to be copyable,
and it has a user-declared destructor; so, follow the Rule of Three!

    struct Animal {
        explicit Animal() = default;
        virtual std::unique_ptr<Animal> clone() const = 0;
        virtual std::string noise() const = 0;
        Animal(const Animal&) = default;
        Animal& operator=(const Animal&) = delete;
        virtual ~Animal() = default;
    };

It might even be worthwhile to mark that copy constructor as `protected`, since
you intend it to be used only by the child class in this very specific scenario.

Notice that `-Wdeprecated` is triggered by the author of `Cat::clone`, but the
fix needs to be made by the author of `Animal`.


### `__declspec(dllexport)`

Someone on the cpplang Slack reported a novel way to accidentally trigger `-⁠Wdeprecated-copy-dtor`
([Godbolt](https://godbolt.org/z/KGorsez8f)): use the Windows compiler
`clang-cl` and mark a polymorphic class with `__declspec(dllexport)`.

When you mark a class with [`dllexport`](https://learn.microsoft.com/en-us/cpp/build/exporting-from-a-dll-using-declspec-dllexport),
the compiler codegens all of its member functions, even the implicitly defaulted special
members that wouldn't ordinarily get instantiated until they were actually used.
This bears some resemblance to explicit template instantiation; see
["Don't explicitly instantiate `std` templates"](/blog/2021/08/06/dont-explicitly-instantiate-std-templates/) (2021-08-06).
The result: you're effectively "using" the deprecated copy operations, even though
you didn't really intend to.

    struct Animal {
        explicit Animal() = default;
        virtual std::string noise() const = 0;
        virtual ~Animal() = default;
    };

    struct __declspec(dllexport) Cat : public Animal {
        explicit Cat() = default;
        std::string noise() const override {
            return "meow";
        }
    };

    warning: definition of implicit copy constructor for 'Animal' is deprecated
    because it has a user-declared destructor [-Wdeprecated-copy-with-dtor]
      virtual ~Animal() = default;
              ^
    note: in implicit copy constructor for 'Animal' first required here
      struct __declspec(dllexport) Cat : public Animal {
                                   ^
    note: in implicit copy constructor for 'Cat' first required here
    note: due to 'Cat' being dllexported
      struct __declspec(dllexport) Cat : public Animal {
                        ^

There are at least three ways you might address this warning:

- `=delete` the copy operations of `Animal`; i.e., convert it from `Base2` style
    to `Base1` style. ([Godbolt.](https://godbolt.org/z/hPcb55Txq))
    This is the best solution, but only the author of `Animal` can do it.

- `=delete` the copy operations of `Cat`. ([Godbolt.](https://godbolt.org/z/T4bP58c5a))
    This is the most surgical solution, but tedious and error-prone if you have dozens
    of derived classes.

- Use `#pragma GCC diagnostic` to suppress the warning around the definition of `Animal`.
    ([Godbolt.](https://godbolt.org/z/38v8xsYsh)) If you can't modify `Animal`,
    then this is the simplest solution, at the cost of a tidbit of compiler-specific cruft.
    In industry codebases, I recommend always wrapping each third-party dependency
    (asio, openssl, protobuf, re2...) in a header file that you control, specifically
    so that you can maintain this kind of cruft inside that single header
    instead of touching dozens of files.

----

Note that GCC trunk has a similar warning `-Wdeprecated-copy-dtor`; but
it's not included in GCC's `-Wdeprecated`, and even when you pass it explicitly, it
seems to trigger only on user-<i>provided</i> destructors like `~B() {}`, not merely
user-<i>declared</i> ones like `~B() = default`. I prefer Clang's behavior, since
it matches the letter of the Standard re what's actually deprecated and therefore re
what might actually change behavior someday in the future.
