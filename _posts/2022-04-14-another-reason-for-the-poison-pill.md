---
layout: post
title: 'PSA: ADL requires that unqualified lookup has found a function'
date: 2022-04-14 00:01:00 +0000
tags:
  argument-dependent-lookup
  customization-points
  library-design
  name-lookup
  pitfalls
  slack
  standard-library-trivia
excerpt: |
  As seen on the [cpplang Slack](https://cppalliance.org/slack/) (hat tip to Jody Hagins).
  Recall my post ["What is the `std::swap` two-step?"](/blog/2020/07/11/the-std-swap-two-step/) (2020-07-11), where I said:

  > A qualified call like `base::frotz(t)` indicates, "I'm sure I know how to `frotz` whatever this thing may be.
  > No type `T` will ever know better than me how to `frotz`."
  >
  > An unqualified call using the two-step, like `using my::xyzzy; xyzzy(t)`, indicates,
  > "I know one way to `xyzzy` whatever this thing may be, but `T` itself might know a better way.
  > If `T` has an opinion, you should trust `T` over me."
  >
  > An unqualified call _not_ using the two-step, like `plugh(t)`, indicates, "Not only should you trust `T`
  > over me, but I myself have no idea how to `plugh` anything. Type `T` _must_ come up with a solution;
  > I offer no guidance here."
---

As seen on the [cpplang Slack](https://cppalliance.org/slack/) (hat tip to Jody Hagins).
Recall my post ["What is the `std::swap` two-step?"](/blog/2020/07/11/the-std-swap-two-step/) (2020-07-11), where I said:

> A qualified call like `base::frotz(t)` indicates, "I'm sure I know how to `frotz` whatever this thing may be.
> No type `T` will ever know better than me how to `frotz`."
>
> An unqualified call using the two-step, like `using my::xyzzy; xyzzy(t)`, indicates,
> "I know one way to `xyzzy` whatever this thing may be, but `T` itself might know a better way.
> If `T` has an opinion, you should trust `T` over me."
>
> An unqualified call _not_ using the two-step, like `plugh(t)`, indicates, "Not only should you trust `T`
> over me, but I myself have no idea how to `plugh` anything. Type `T` _must_ come up with a solution;
> I offer no guidance here."

Several places in the C++20 STL use that third approach. For example, [`std::ranges::begin`](http://eel.is/c++draft/ranges#range.access.begin)
tries an unqualified call to `begin(t)`; if `begin(t)` is ill-formed, `std::ranges::begin` does _not_
fall back to `std::begin`. Likewise, `std::strong_order` tries an unqualified call to `strong_order(t, t)`,
without keeping `any_other::strong_order` as a fallback. And it's not just C++20's CPOs that use this pattern;
the constructor of C++11's `std::error_code` relies on an unqualified call to `make_error_code(t)`.

But if you're expecting your library's unqualified call to `f(t)` to trigger ADL, you must contend
with [[basic.lookup.argdep]/1](https://eel.is/c++draft/basic.lookup.argdep#1), which says that if
your initial unqualified lookup of `f` finds a non-function (or a function declaration
in a block scope) then ADL doesn't take place. If unqualified lookup finds a function, or a function template,
or nothing at all, then you get ADL; if it finds a non-function, you get nothing!

----

As of this writing, libc++ (but not libstdc++ or MSVC) can be tricked into stumbling on `make_error_code`
([Godbolt](https://godbolt.org/z/K8qoja7n4)):

    int make_error_code;  // ha ha!

    #include <system_error>

    namespace N {
        enum E { RED, YELLOW, BLUE };
        std::error_code make_error_code(E);
    }
    template<>
    struct std::is_error_code_enum<N::E> : std::true_type {};

    int main() {
        std::error_code e = N::E();
    }

The compiler complains:

    include/c++/v1/system_error:330:22: error: called object type 'int'
    is not a function or function pointer
            {*this = make_error_code(__e);}
                     ^~~~~~~~~~~~~~~
    note: in instantiation of function template specialization
    'std::error_code::error_code<N::E>' requested here
        std::error_code e = N::E();
                            ^

----

Also as of this writing, libc++ and libstdc++ (but not MSVC) can be tricked into stumbling on `strong_order`
([Godbolt](https://godbolt.org/z/M6fqxve34)):

    int strong_order;  // ha ha!

    #include <compare>

    namespace N {
        struct S {
            friend auto strong_order(S, S) { ~~~ }
        };
    }

    auto x = std::strong_order(N::S(), N::S());

The compiler complains:

    error: no matching function for call to object of type
    'const __strong_order::__fn'
    auto x = std::strong_order(N::S(), N::S());
             ^~~~~~~~~~~~~~~~~
    __compare/strong_order.h:120:56: note: candidate template ignored:
    constraints not satisfied [with _Tp = N::S, _Up = N::S]
        decltype(auto) operator()(_Tp&& __t, _Up&& __u) const
                       ^
    __compare/strong_order.h:124:44: note: because
    'std::forward<_Tp>(__t) <=> std::forward<_Up>(__u)' would be invalid:
    invalid operands to binary expression ('N::S' and 'N::S')
            std::forward<_Tp>(__t) <=> std::forward<_Up>(__u);
                                   ^
    __compare/strong_order.h:131:21: note: and
    'strong_order(std::forward<_Tp>(__t), std::forward<_Up>(__u))' would be invalid:
    called object type 'int' is not a function or function pointer
            strong_order(std::forward<_Tp>(__t), std::forward<_Up>(__u));
            ^

The `strong_order` found by unqualified lookup is the evil user's `int strong_order`
variable, so Clang is quite right: `strong_order` is not callable, and no ADL
takes place.

----

Hui Xie points out that you can break things even harder by making the evil
declaration a namespace declaration:

    namespace strong_order {}
    namespace make_error_code {}
    #include <system_error>

and that Boost.Graph is a non-STL library containing at least one example
of this problem ([Godbolt](https://godbolt.org/z/s5Tn5Wdfd)):

    namespace target {}
    #include <boost/graph/graph_concepts.hpp>

----

Most Ranges CPOs, such as `ranges::begin`, specify that the ADL lookup is
"performed in a context in which unqualified lookup for `begin` finds..." some
specific function declarations. In the case of `ranges::begin`, those are:

    void begin(auto&) = delete;
    void begin(const auto&) = delete;

The primary motivation for these "poison pill" declarations (as I understand it)
is to prevent overload resolution from considering `std::begin` as a candidate
via ordinary unqualified lookup (even though we would ordinarily expect a lookup
starting in namespace `std::ranges` to find `std::begin`). A secondary effect
(and the only reason, as far as I know, that motivates the exact signature `void begin(auto&)`)
is to prevent overload resolution from considering "insufficiently constrained"
_user-defined_ templates such as `Bad`'s friend below ([Godbolt](https://godbolt.org/z/odvYfzTG1)):

    struct OK {
        friend int *begin(OK&);
    } ok;
    auto x = std::ranges::begin(ok); // OK

    struct Bad {
        friend int *begin(auto&);
    } bad;
    auto y = std::ranges::begin(bad); // ERROR!

But for this blog post's purposes, the third and most important effect of a "poison pill"
declaration is to make sure that unqualified lookup finds a
function declaration — instead of searching all the way out to the
global scope where an evil user might have declared a non-function with that
name, thus preventing ADL.

Library implementors take note: bare unqualified ADL is probably a bad idea!
If you're not using the `std::swap` two-step, then use a "poison pill" declaration
to ensure that your unqualified lookup never finds a non-function.

----

See also:

* ["Why is deleting a function necessary when you're defining a CPO?"](https://stackoverflow.com/a/63548215/1424877) on StackOverflow (Barry Revzin, August 2020)

* microsoft/STL #1374: ["Spaceship CPO wording in [cmp.alg] needs an overhaul"](https://github.com/microsoft/STL/issues/1374)

* [LWG3629 "`make_error_code` and `make_error_condition` are customization points"](https://cplusplus.github.io/LWG/issue3629)
