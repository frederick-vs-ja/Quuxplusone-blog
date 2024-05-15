---
layout: post
title: '`mutable` and "trivially fooable"'
date: 2018-08-19 00:01:00 +0000
tags:
  relocatability
  triviality
  wg21
excerpt: |
  Pop quiz, hotshot.

      struct M {
          M() = default;
          M(M&);
          M(const M&) = default;
      };

  Is `M` trivially move-constructible?
---

Pop quiz, hotshot.

    struct M {
        M() = default;
        M(M&);
        M(const M&) = default;
    };

Is `M` trivially move-constructible?

(Important background info: The C++ Standard considers [both](http://eel.is/c++draft/class.copy.ctor#1)
`M(const M&)` and `M(M&)` to be "copy constructors.")

Clang and ICC (that's the Intel C++ Compiler, which uses the EDG front-end) both say
yes `M` is trivially move-constructible, because when you move-construct an `M`,
you call the `M(const M&)` constructor, which is defaulted and thus (in this case) trivial.

GCC says [no it isn't](https://godbolt.org/z/-sBYqO).
(UPDATE: [I've filed this bug.](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87051))

MSVC correctly surmises that in this game, the only winning move is to sit in the corner and eat paste:

> error C2580: `M::M(const M &)`:
> multiple versions of a defaulted special member functions are not allowed

----

This quiz turns out to be tangentially important to P1144 "Object relocation in terms of move plus destroy,"
because it exposes a case where the intuitive "rules of inheritance" with regard to trivial fooability
do not apply.

    struct N1 {
        mutable M m;
    };

Assuming that we believe Clang and ICC (which I do) that `M` is trivially move-constructible, then
so is `N1`.  However,

    struct N2 {
        mutable M m;
        N2(const N2&) = default;
    };

`N2` definitely is *not* trivially move-constructible, because if you try to move-construct an `N2`,
it will call `N2(const N2&)`, which will call `M(M&)` on the mutable member, which is user-provided.

----

So I've been vacillating on what the rules should be that decide if a class type is "naturally" trivially
relocatable (that is, if the class type isn't marked with the `[[trivially_relocatable]]` attribute).

[Draft revision 7](/blog/code/object-relocation-in-terms-of-move-plus-destroy-draft-7.html)
as previously posted [on this blog](/blog/2018/07/18/announcing-trivially-relocatable/) said:

> A move-constructible, destructible object type `T` is a _trivially relocatable type_ if it is:
>
> - a (possibly cv-qualified) class type declared with the `[[trivially_relocatable]]` attribute, or
>
> - a (possibly cv-qualified) class type which:
>
>   - has either a defaulted, non-deleted move constructor or no move constructor and a defaulted, non-deleted copy constructor,
>
>   - has no `mutable` members,
>
>   - etc. etc.

In [draft revision 12](https://groups.google.com/a/isocpp.org/d/msg/sg14/6mAbZOTdVjk/wuH3qZhKAAAJ) I changed that to:

> A move-constructible, destructible object type `T` is a _trivially relocatable type_ if it is:
>
> - a (possibly cv-qualified) class type declared with the `[[trivially_relocatable]]` attribute, or
>
> - a (possibly cv-qualified) class type which:
>
>   - either has a defaulted, non-deleted move constructor, or
>         has no move constructor, a defaulted, non-deleted copy constructor,
>         and no `mutable` or `volatile` subobjects,
>
>   - etc. etc.

In draft revision 13 I'm considering changing it again to:

> A move-constructible, destructible object type `T` is a _trivially relocatable type_ if it is:
>
> - a (possibly cv-qualified) class type declared with the `[[trivially_relocatable]]` attribute, or
>
> - a (possibly cv-qualified) class type which:
>
>   - has no deleted or user-provided move constructors,
>
>   - has no deleted or user-provided copy constructors,
>
>   - etc. etc.

Thoughts welcome!
