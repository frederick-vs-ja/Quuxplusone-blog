---
layout: post
title: "Fun with lifetime-extended results of assignment"
date: 2022-07-09 00:01:00 +0000
tags:
  implementation-divergence
  lifetime-extension
excerpt: |
  Previously on this blog: ["Fun with conversion-operator lookup"](/blog/2021/01/13/conversion-operator-lookup/) (2021-01-13).

  Here's another simple C++ program that gives different output on three of the four
  mainstream compilers ([Godbolt](https://godbolt.org/z/MrP77h7ov)):

      #include <stdio.h>
      char messages[][23] = {
        "This is Clang (or EDG)",
        "This is GCC",
        "This is MSVC",
      };
      int count;
      struct X { ~X() { ++count; } };

      int main() {
        {
          X& a = (X() = X());
          count = 0;
        }
        puts(messages[count]);
      }
---

Previously on this blog: ["Fun with conversion-operator lookup"](/blog/2021/01/13/conversion-operator-lookup/) (2021-01-13).

Here's another simple C++ program that gives different output on three of the four
mainstream compilers ([Godbolt](https://godbolt.org/z/MrP77h7ov)):

    #include <stdio.h>
    char messages[][23] = {
      "This is Clang (or EDG)",
      "This is GCC",
      "This is MSVC",
    };
    int count;
    struct X { ~X() { ++count; } };

    int main() {
      {
        X& a = (X() = X());
        count = 0;
      }
      puts(messages[count]);
    }

As far as I can tell, Clang and EDG are correct; the other two are buggy.

----

We create two `X` objects in this program: the temporary on the left-hand side
of the assignment operator, and the temporary on the right-hand side.
MSVC seems to be doing a sort of "lifetime extension" of both objects all the
way to the end of `a`'s scope. [[class.temporary]/6](https://eel.is/c++draft/class.temporary#6)
doesn't seem to offer any textual justification for extending the left-hand
operand, let alone the right-hand operand.

GCC seems to be extending the lifetime of the left-hand
`X()` but not the right-hand one. (We can tell by instrumenting the constructors:
[Godbolt](https://godbolt.org/z/YMzT3fqT6).) But here's the weird part:
GCC does this lifetime extension only when `struct X` has no named fields!
That is, GCC will lifetime-extend the left-hand operand of an assignment when its
type is any of these:

    struct alignas(4) X { ~X(); };
    struct X { int :0; ~X(); };
    struct X { int :16; ~X(); };

but not when its type is either of these:

    struct X { int i:1; ~X(); };
    struct X { char c; ~X(); };

> Incidentally, MSVC is the only vendor that allows `struct X { int :16; }`
> to qualify as `std::is_empty`. I think MSVC is correct (and Clang/GCC are wrong):
> such an `X` certainly has [no non-static data members](https://eel.is/c++draft/meta.unary#tab:meta.unary.prop-row-7-column-2-sentence-1),
> since [unnamed bit-fields are not members](https://eel.is/c++draft/class.bit#2.sentence-2) at all.

Finally, all this wackiness manifests only when `X::operator=` is defaulted
(either implicitly, as shown here, or explicitly via `=default`). If you
provide your own user-defined

    X& operator=(X&&) { return *this; }

then the divergence vanishes: all vendors agree that there shouldn't be
any lifetime extension in that case.
