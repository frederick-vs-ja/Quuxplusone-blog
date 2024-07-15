---
layout: post
title: "Why can't I specialize `std::hash` inside my own namespace?"
date: 2024-07-15 00:01:00 +0000
tags:
  cpplang-slack
  name-lookup
  proposal
  templates
---

This question comes up a lot on [the cpplang Slack](https://cppalliance.org/slack/).
Suppose I have a class named `my::Book`, and I want to put it into a `std::unordered_set`.
Then I need to write a `std::hash` specialization for it. So I write:

    namespace my {
      struct Book { ~~~~ }; // A
      struct Library { ~~~~ };
    } // namespace my

    template<>
    struct std::hash<my::Book> { // D
      size_t operator()(const my::Book& b) const {
        return b.hash();
      }
    };

> See ["Don't reopen `namespace std`"](/blog/2021/10/27/dont-reopen-namespace-std/) (2021-10-27).

But that's a lot of extra `my::`-qualification, and (even worse) requires that I _remember_ from line `A` all the
way down to line `D` that I need to specialize `hash` for my type. I'd vastly prefer to provide the
implementation of `hash<Book>` right next to `Book` itself, like this:

    namespace my {
      struct Book { ~~~~ }; // A

      template<>
      struct std::hash<Book> { // D
        size_t operator()(const Book& b) const {
          return b.hash();
        }
      };

      struct Library { ~~~~ };
    } // namespace my

Sadly, C++ doesn't let us do this. There have been at least two WG21 proposals to allow this,
but both were abandoned:

* [N3730](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3730.html) and [N3867 "Specializations and namespaces"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3867.html) (Mike Spertus, 2014)
* [P0665 "Allow Class Template Specializations in Associated Namespaces"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0665r1.pdf) (Tristan Brindle, 2018)

See also [CWG374 "Can explicit specialization outside namespace use qualified name?"](https://cplusplus.github.io/CWG/issues/374.html),
[N3064 "Explicit specialization outside a template's parent,"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2010/n3064.pdf)
[CWG1077 "Explicit specializations in non-containing namespaces,"](https://cplusplus.github.io/CWG/issues/1077.html) and
[EWG48 "Specializations and namespaces."](https://cplusplus.github.io/EWG/ewg-active.html#48)

## Analogous case for member templates

The status quo is that `std::hash` can be specialized only "in any scope in which the corresponding primary template may be defined"
(<a href="https://eel.is/c++draft/temp.expl.spec#3">[temp.expl.spec]/3</a>, <a href="https://eel.is/c++draft/temp.spec.partial.general#6">[temp.spec.partial.general]/6</a>).
This wording originated in response to [CWG727 "In-class explicit specializations"](https://cplusplus.github.io/CWG/issues/727.html) (2008) —
see also [CWG1755 "Out-of-class partial specializations of member templates"](https://cplusplus.github.io/CWG/issues/1755.html) and
[N4090](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4090.pdf) — where the problem _they_ were all thinking about was
the problem of _member_ templates. We certainly don't want to permit e.g.

    struct A {
      template<class> struct Hash;
    };
    struct B {
      template<> struct A::Hash<int> {}; // error
    };

Nor do we want to permit:

    struct A {
      template<class> struct Hash;
      struct B {
        template<> struct Hash<int> {}; // error
      };
    };

> Incidentally, GCC doesn't support explicit specializations in member scope at all;
> that's [GCC bug #85282](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85282).

So, one might ask, why should it work any differently when `A` and `B` are namespaces rather than classes?

---

On the other hand, clearly it _does_ work differently for namespaces versus classes. Consider these two
perfectly parallel snippets:

    struct A {
      struct B {
        template<class> struct Hash;
      };
      template<> struct B::Hash<int> {}; // error
    };

    namespace A {
      namespace B {
        template<class> struct Hash;
      }
      template<> struct B::Hash<int> {}; // OK
    }

The former is ill-formed (which seems to me like an excellent idea).
The latter is OK; in fact, it's exactly how we specialize `std::hash` today.

So there's nothing terribly inconsistent with our wanting to treat _these_ two snippets differently also:

    struct A {
      template<class> struct Hash;
    };
    struct B {
      template<> struct A::Hash<int> {}; // error; we'd like to keep it an error
    };

    namespace A {
      template<class> struct Hash;
    }
    namespace B {
      template<> struct A::Hash<int> {}; // error, but we'd like to make it OK
    }

## The problem is name lookup

The real problem is name lookup. Consider ([Godbolt](https://godbolt.org/z/b11bnKKTW)):

    namespace A {
      int f() { return 1; }
      template<class> struct Hash;
    }
    int f() { return 2; }
    template<> struct A::Hash<int> {
      int g() { return f(); } // C
    };

The call on line `C` finds `A::f`, not `::f`, because there, although that line is _lexically_
inside the global namespace, it is also inside a specialization of `A::Hash` and thus _logically_
inside namespace `A`. The same happens if we replace the class templates with function templates
([Godbolt](https://godbolt.org/z/9PMdEv3xs)).

Now, what happens if we make this code legal:

    namespace A {
      int f() { return 1; }
      template<class> struct Hash;
    }
    namespace B {
      int f() { return 2; }
      template<> struct A::Hash<int> {
        int g() { return f(); } // C
      };
    }

Does line `C` call `A::f` (because we're logically inside a specialization of `A::Hash`),
or `B::f` (because we're lexically inside `namespace B`)? Obviously, by the above logic, it must
call `A::f`. But consider how awkward this would be in practice:

    namespace my {
      struct Book { ~~~~ };

      template<>
      struct std::hash<Book> { // D
        size_t operator()(const my::Book& b) const { // E
          ~~~~
        }
      };
    }

On line `D` we can say `Book`; but on line `E` we must say `my::Book`, because at that point we're
logically inside namespace `std` and a lookup for `Book` inside namespace `std` wouldn't find anything.
Worse, if its fully qualified name is something like `mycompany::my::detail::Book`, we'll have to
spell out that whole thing!

### A potentially dangerous pitfall

If your type in namespace `my` shares its name with something in `std`, this gotcha might
really hurt. For example:

    namespace my {
      template<class T>
      struct vector { ~~~~ };

      template<class T>
      struct std::hash<vector<T>> {                  // D
        size_t operator()(const vector<T>& v) const; // E
      };
    }

By the name-lookup logic above, line `D` means `my::vector<T>` but line `E` means `std::vector<T>`.
This could lead to very confusing error messages later in the program —
or worse, runtime misbehavior, if `my::vector` is implicitly convertible to `std::vector`!

> Note a similar pitfall with variable initializers ([Godbolt](https://godbolt.org/z/hMWf9f8zv)):
>
>     namespace A { extern int g; }
>     namespace A { int f() { return 1; } }
>
>     int f() { return 2; }
>     int A::g = f();
>
> initializes `A::g` to `1`, not `2`. I think any working programmer would be
> shocked by that result; but it never comes up in practice.

### An almost-workaround

Here's _almost_ (but not quite) a clever workaround for the above deficiency.
We're already delegating the body of `std::hash<Book>::operator()` to `Book::hash()`, so that
its code appears in the correct logical scope (`my` rather than `std`). Could we
just delegate a little more?

    namespace my {
      struct Book {
        struct Hash {
          size_t operator()(const Book& b) const { ~~~~ } // E
        };
      };
      template<> struct std::hash<Book> // D
        : my::Book::Hash {};            // F
    }

Now we can use the unqualified name `Book` on both lines `D` and `E`. But it turns out
that the _base-clause_ on line `F` is also logically within namespace `std`,
not namespace `my` (<a href="https://eel.is/c++draft/basic.scope.class#1.sentence-2">[basic.scope.class]/1</a>),
so on line `F` we still need to spell out `my::Book` with full qualification.
Factoring out `Book::Hash` hasn't gained us much!


## Conclusion

I'd like to see C++ gain the ability to define specializations of `std::hash` lexically
within `namespace my`. Despite the downside of name lookup's requiring fully qualified names
(and the pitfall above when you forget full qualification in a critical place),
the ergonomic benefits would still be enormous.

Still, it would certainly be much easier to sell the feature if it didn't have that
pitfall. Do you have an idea that would solve the name-lookup pitfall — without breaking
any of the other examples in this post? If you do, please, contact me via the
[email link](mailto:arthur.j.odwyer@gmail.com) below!
