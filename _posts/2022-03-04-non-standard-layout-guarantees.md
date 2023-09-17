---
layout: post
title: 'History of non-standard-layout class layouts'
date: 2022-03-04 00:01:00 +0000
tags:
  access-control
  wg21-folkloristics
---

> Thanks to Jody Hagins for inspiring this blog post, for digging up some
> old wordings, and for reviewing a draft of this post. Also thanks to Pal Balog
> for P1847; its historical details were indispensable in writing this post.

In C++, we have the notion of "standard layout." A standard-layout class type
is basically guaranteed to be laid out in memory the same way as a plain old
C struct: the first data member goes at offset zero, and subsequent members
are laid out at increasing addresses in declaration order (possibly with some
gaps for padding).

    struct A { int i; int j; } a;
    struct B { int m; int n; };

Both C and C++ have always guaranteed that `(int*)&a == &a.i`, and even
that `offsetof(A, j) == offsetof(B, n)`. These guarantees are due to the fact
that `A` is a [standard-layout](https://eel.is/c++draft/class.prop#3) type
that's [layout-compatible](https://eel.is/c++draft/class.mem#general-23)
with `B`. The rules for what kinds of classes count as standard-layout have
drifted a bit over the years
(see [cppreference](https://en.cppreference.com/w/cpp/named_req/StandardLayoutType)),
certainly anything that would have compiled in C89 will count as
standard-layout in C++ forever.

However, there is a second layout guarantee in C++ that applies to _all_ class types,
regardless of whether they're standard-layout or not!
Consider the following class type ([Godbolt](https://godbolt.org/z/oaPxfbzhK)):

    struct C {
        int a;
        int b;
    private:
        int c;
    public:
        int d;
    } c;
    static_assert(&c.a < &c.b);

Ever since C++98, it's been guaranteed that `&c.a < &c.b`.
C++98's [expr.rel] (page 86):

> If two pointers point to nonstatic data members of the same object,
> or to subobjects or array elements of such members, recursively,
> the pointer to the later declared member compares greater
> provided the two members are not separated by an access-specifier label
> and provided their class is not a union.

This is consistent with the idea that `a.i` must precede `a.j` in the
memory layout of the [POD](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#pod)
type `A`, but it's even stronger — because it also governs
the layout of non-POD types like `C`.

Notice that in C++98, there was no guarantee that `&c.b < &c.c`, nor that
`&c.b < &c.d`, because those members' declarations _are_ separated by
access-specifier labels. C++98 guaranteed no reordering within a single
access specifier's "block" of data members, but still permitted reordering
among those "blocks." For example, the compiler might (if it wished) lay out
`C`'s members in the order `a b d c` (private members at the back); or `c a b d`;
or even `d c a b` (blocks ordered back to front). A really evil implementation
might even order them as `a c d b`, since that technically conforms to the guarantee.

Between C++98 and C++11, the notion of "POD type" was replaced with the conjunction
of "standard-layout type" and "trivial type," and in the process the layout guarantee was
strengthened to enable the programmer to interrupt a run of public members with some
private member functions, or vice versa, without breaking ABI. That is, C++11 forbid
the compiler to swap two "blocks" of data members with the same access. Since `C`'s
members `a`, `b`, `d` are all public, C++11 required the compiler to lay them out
in exactly that order — but still didn't restrict the placement of `c` relative
to the public members.

Finally, in C++23, [P1847R4 "Make declaration order mandated"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1847r4.pdf)
removes _all_ of the compiler's freedom to reorder the layout of data members... at least,
for data members declared directly within the same class. I believe (and Clang agrees)
that the standard still says nothing about inheritance situations such as:

    struct Base { int i; };
    struct Derived : Base { int j; } d;
    static_assert(&d.i < &d.j); // Not guaranteed even in C++23

Here's a table summarizing the history of the standard's layout guarantees for
non-standard-layout types like `C`.
Please keep in mind that this table's "Allowed" column is _100% purely hypothetical_ —
it's what the [DeathStation 9000](https://web.archive.org/web/20150619140401/https://en.wikipedia.org/wiki/User:CompuHacker/CHDS9000)
might have chosen to do. No actual shipping compiler has ever done any of these
things, as far as I know (except for one near miss, mentioned in the numbered note below).

<table class="smaller">
<tr>
  <th>C++ version</th>
  <th>[expr.rel]</th>
  <th>[class.mem]</th>
  <th>Permitted for <code>C</code></th>
  <th>Forbidden for <code>C</code></th>
</tr>
<tr>
  <td rowspan="2">
    C++98 (<a href="https://web.archive.org/web/20160910082028/http://www.lirmm.fr/~ducour/Doc-objets/ISO+IEC+14882-1998.pdf">ISO/IEC 14882:1998</a>)<br/><br/>
    C++03 (<a href="https://web.archive.org/web/20220225224452/http://staff.ustc.edu.cn/~zhuang/cpp/specs/ISO_IEC%2014882%202003.pdf">ISO/IEC 14882:2003</a>)
  </td>
  <td style="min-width: 16em;">
    If two pointers point to nonstatic data members of the same object,
    or to subobjects or array elements of such members, recursively,
    the pointer to the later declared member compares greater
    provided the two members are not separated by an access-specifier label
    and provided their class is not a union.
  </td>
  <td style="min-width: 16em;">
    Nonstatic data members of a (non-union) class declared without an intervening access-specifier
    are allocated so that later members have higher addresses within a class object.
    The order of allocation of nonstatic data members separated by an access-specifier is unspecified.
  </td>
  <td style="text-align: center;">abcd&nbsp;abdc<sup>[1]</sup><br/>acbd&nbsp;acdb<br/>adbc&nbsp;adcb<br/>cabd&nbsp;cadb<br/>cdab&nbsp;dabc<br/>dacb&nbsp;dcab</td>
  <td style="text-align: center;">bacd&nbsp;badc<br/>bcad&nbsp;bcda<br/>bdac&nbsp;bdca<br/>cbad&nbsp;cbda<br/>cdba&nbsp;dbac<br/>dbca&nbsp;dcba</td>
</tr>
<tr>
  <td colspan="4">
    [1] — <i>According to P1847, EDG's frontend does support this layout
    (all public members first, then protected, then private), but only under a
    build-time configuration flag that no customer of theirs has ever used.</i>
  </td>
</tr>
<tr>
  <td colspan="5">
    <a href="https://cplusplus.github.io/CWG/issues/568.html">CWG 568 "Definition of POD is too strict"</a><br/>
    <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2342.htm">N2342 "PODs Revisited"</a> (i.e. splitting up "POD" into "standard-layout and trivial")<br/>
  </td>
</tr>
<tr>
  <td>C++11 (<a href="https://timsong-cpp.github.io/cppwp/n3337">N3337</a>)</td>
  <td style="min-width: 16em;">
    If two pointers point to non-static data members of the same object,
    or to subobjects or array elements of such members, recursively,
    the pointer to the later declared member compares greater
    provided the two members have the same access control
    and provided their class is not a union.
  </td>
  <td style="min-width: 16em;">
    Nonstatic data members of a (non-union) class with the same access control
    are allocated so that later members have higher addresses within a class object.
    The order of allocation of non-static data members with different access control is unspecified.
  </td>
  <td style="text-align: center;">abcd<br/>abdc<br/>acbd<br/>cabd</td>
  <td style="text-align: center;"><i>Everything else</i></td>
</tr>
<tr>
  <td colspan="5">
    <a href="https://cplusplus.github.io/CWG/issues/1512.html">CWG 1512 "Pointer comparison vs qualification conversions"</a><br/>
    <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3624.html">N3624 "Pointer comparison vs qualification conversions"</a>
  </td>
</tr>
<tr>
  <td>
    C++14 (<a href="https://timsong-cpp.github.io/cppwp/n4140">N4140</a>)<br/><br/>
    C++17 (<a href="https://timsong-cpp.github.io/cppwp/n4659">N4659</a>)
  </td>
  <td style="min-width: 16em;">
    If two pointers point to different non-static data members of the same object,
    or to subobjects of such members, recursively,
    the pointer to the later declared member compares greater
    provided the two members have the same access control
    and provided their class is not a union.
  </td>
  <td style="min-width: 16em;">
    Nonstatic data members of a (non-union) class with the same access control
    are allocated so that later members have higher addresses within a class object.
    The order of allocation of non-static data members with different access control is unspecified.
  </td>
  <td style="text-align: center;">abcd<br/>abdc<br/>acbd<br/>cabd</td>
  <td style="text-align: center;"><i>Everything else</i></td>
</tr>
<tr>
  <td colspan="5">
    <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0840r2.html">P840R2 "Language support for empty objects"</a> (i.e., <code>[[no_unique_address]]</code>)<br/>
    <a href="https://cplusplus.github.io/CWG/issues/2404.html">CWG 2404 "[[no_unique_address]] and allocation order"</a><br/>
    <a href="https://github.com/cplusplus/draft/pull/1977">Editorial: Clarify auxiliary partial ordering</a>
  </td>
</tr>
<tr>
  <td>C++20 (<a href="https://timsong-cpp.github.io/cppwp/n4868">N4868</a>)</td>
  <td style="min-width: 16em;">
    If two pointers point to different non-static data members of the same object,
    or to subobjects of such members, recursively,
    the pointer to the later declared member is required to compare greater
    provided the two members have the same access control,
    neither member is a subobject of zero size, and their class is not a union.
  </td>
  <td style="min-width: 16em;">
    Note: <i>Non-static data members of a (non-union) class with the same access control and non-zero size
    are allocated so that later members have higher addresses within a class object.
    The order of allocation of non-static data members with different access control is unspecified.</i>
  </td>
  <td style="text-align: center;">abcd<br/>abdc<br/>acbd<br/>cabd</td>
  <td style="text-align: center;"><i>Everything else</i></td>
</tr>
<tr>
  <td colspan="5">
    <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1847r4.pdf">P1847R4 "Make declaration order mandated"</a>
  </td>
</tr>
<tr>
  <td>C++2b</td>
  <td style="min-width: 16em;">
    If two pointers point to different non-static data members of the same object,
    or to subobjects of such members, recursively,
    the pointer to the later declared member is required to compare greater
    provided neither member is a subobject of zero size and their class is not a union.
  </td>
  <td style="min-width: 16em;">
    Note: <i>Non-variant non-static data members of non-zero size
    are allocated so that later members have higher addresses within a class object.</i>
  </td>
  <td style="text-align: center;">abcd</td>
  <td style="text-align: center;"><i>Everything else</i></td>
</tr>
</table>
