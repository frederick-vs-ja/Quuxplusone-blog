---
layout: post
title: "Should assignment affect `is_trivially_relocatable`?"
date: 2024-01-02 00:01:00 +0000
tags:
  proposal
  relocatability
excerpt: |
  Consider the following piece of code that uses Bloomberg's `bsl::vector`:

      #include <bsl_vector.h>
      using namespace BloombergLP::bslmf;

      struct T1 {
          int i_;
          T1(int i) : i_(i) {}
          T1(const T1&) = default;
          void operator=(const T1&) { puts("Assigned"); }
          ~T1() = default;
      };
      static_assert(!IsBitwiseMoveable<T1>::value);

      int main() {
          bsl::vector<T1> v = {1,2};
          v.erase(v.begin());
      }

  `bsl::vector::erase` uses `T1::operator=` to shift element `2` leftward into
  the position of element `1` (and then destroys the moved-from object in position `2`).
  The program prints "Assigned."

  But that's because type `T1` is not trivially relocatable!
---

Consider the following piece of code that uses Bloomberg's `bsl::vector`:

    #include <bsl_vector.h>
    using namespace BloombergLP::bslmf;

    struct T1 {
        int i_;
        T1(int i) : i_(i) {}
        T1(const T1&) = default;
        void operator=(const T1&) { puts("Assigned"); }
        ~T1() = default;
    };
    static_assert(!IsBitwiseMoveable<T1>::value);

    int main() {
        bsl::vector<T1> v = {1,2};
        v.erase(v.begin());
    }

`bsl::vector::erase` uses `T1::operator=` to shift element `2` leftward into
the position of element `1` (and then destroys the moved-from object in position `2`).
The program prints "Assigned."

But that's because type `T1` is not trivially relocatable!

> BSL calls the relevant trait `IsBitwiseMoveable`, not `IsTriviallyRelocatable`;
> but that's only because their codebase predates the C++11 meaning of "move."
> Qt, a codebase of similar antiquity, upgraded their own terminology
> from "movable" to "relocatable" only in
> [November 2020](https://github.com/qt/qtbase/commit/0440614af0bb08e373d8e3e40f90b6412c043d14).
>
> I don't give a Godbolt link here only because BSL isn't available on Godbolt
> yet: [#5933](https://github.com/compiler-explorer/compiler-explorer/issues/5933).

Let's try the same thing with a trivially relocatable type `T2`:

    struct T2 : NestedTraitDeclaration<T2, IsBitwiseMoveable> {
        int i_;
        T2(int i) : i_(i) {}
        T2(const T2&) = default;
        void operator=(const T2&) { puts("Assigned"); }
        ~T2() = default;
    };
    static_assert(IsBitwiseMoveable<T2>::value);

    int main() {
        bsl::vector<T2> v = {1,2};
        v.erase(v.begin());
    }

Now BSL knows that elements of type `T2` can be trivially (that is, bitwise)
relocated; so `bsl::vector::erase` destroys element `1` and then uses `memmove`
to shift element `2` leftward into that vacant position.
The program prints nothing; `T2::operator=` is never called.

Okay, what's the moral of this story?

- If you warrant a type as "trivially relocatable," you must be prepared for library-writers
    not only to skip some construct/destroy pairs, but also to skip some assignment operations.

- To put it from the library-writer's point of view: Every trivially relocatable type has a "sane"
    assignment operator. Assigning a trivially relocatable type means no more or less than
    transferring its value. Library-writers _can and do_ optimize based on this fact.

- To put it from the P1144 compiler's point of view: Suppose we have a type like `T1` that
    appears perfectly trivially relocatable _except_ that it has a user-provided `operator=`.
    That user-provided assignment operator could do anything. We must consider the type
    non-trivially-relocatable. (Indeed, `std::is_trivially_relocatable_v<T1>` [is false](https://godbolt.org/z/GPb1WzKaY),
    for the same reason that `bslmf::IsBitwiseMoveable<T1>` is false.)

Now, of course a type with a customized assignment operator can be _explicitly warranted_
as trivially relocatable; that's exactly what we do in `T2` (using BSL's library-based opt-in).
Here's the same example using Qt's library syntax for the warrant ([Godbolt](https://godbolt.org/z/aMWGT54eh)):

    #include <QList>

    struct T2 {
        int i_;
        T2(int i) : i_(i) {}
        T2(const T2&) = default;
        void operator=(const T2&) { puts("Assigned"); }
        ~T2() = default;
    };
    Q_DECLARE_TYPEINFO(T2, Q_RELOCATABLE_TYPE);

    static_assert(QTypeInfo<T2>::isRelocatable);

    int main() {
        QList<T2> v = {1,2,3};
        v.erase(v.begin() + 1);
          // does not print "Assigned"
    }

And using P1144's syntax for the warrant:

    struct [[trivially_relocatable]] T2 {
        int i_;
        T2(int i) : i_(i) {}
        T2(const T2&) = default;
        void operator=(const T2&) { puts("Assigned"); }
        ~T2() = default;
    };
    static_assert(std::is_trivially_relocatable<T2>::value);

As of this writing, my libc++ fork follows BSL-and-Qt's lead in optimizing `vector::erase` for trivially relocatable types.
My newer libstdc++ fork does not, yet; but eventually it will. ([Godbolt.](https://godbolt.org/z/qa79Gf369))

----

"Wait, isn't it technically non-conforming to use anything but assignment in `vector::erase`, because
[`vector::erase`'s _Complexity_ element](https://eel.is/c++draft/vector.modifiers#5) specifically requires
the assignment operator of `T` to be called a certain number of times?" — Well, yes, you've got me there,
for now. But we (STL) would really like it to be conforming, because we (BSL, Qt, Folly, ...) already do it!
WG21 likes to say that C++ should "leave no room for a lower-level language"; it doesn't really make sense
that a valuable optimization that every third-party library vendor already does should be forbidden
to `std::vector` on a technicality.

My [P3055R0 "Relax wording to permit relocation optimizations in the STL"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p3055r0.html)
(December 2023) aims to patch this hole. [D3055R1](https://quuxplusone.github.io/draft/d3055-relocation.html)
adds a few "stretch goal" patches on top of R0. Feedback welcome; send me an email!
