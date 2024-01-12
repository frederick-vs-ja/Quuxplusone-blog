---
layout: post
title: "Pointer comparisons with `std::less<void>`: a horror story"
date: 2019-01-20 00:01:00 +0000
tags:
  metaprogramming
  operator-spaceship
  rant
  standard-library-trivia
  undefined-behavior
excerpt: |
  From C++ standard section [[comparisons]/2](http://eel.is/c++draft/comparisons#2):

  > For templates `less`, `greater`, `less_equal`, and `greater_equal`, the specializations
  > for any pointer type yield a strict total order that is consistent among those specializations
  > and is also consistent with the partial order imposed by the built-in operators `<`, `>`, `<=`, `>=`.
  > For template specializations `less<void>`, `greater<void>`, [et cetera], if the call operator
  > calls a built-in operator comparing pointers, the call operator yields a strict total order [...]

  "Wait, what partial order? Are pointers not totally ordered in C++?"
---

From C++ standard section [[comparisons]/2](http://eel.is/c++draft/comparisons#2):

> For templates `less`, `greater`, `less_equal`, and `greater_equal`, the specializations
> for any pointer type yield a strict total order that is consistent among those specializations
> and is also consistent with the partial order imposed by the built-in operators `<`, `>`, `<=`, `>=`.
> For template specializations `less<void>`, `greater<void>`, [et cetera], if the call operator
> calls a built-in operator comparing pointers, the call operator yields a strict total order [...]

"Wait, what partial order? Are pointers not totally ordered in C++?"

That's (unfortunately) correct. Given an array `a`, the standard language assures us that
`&a[i] < &a[j]` whenever `i < j`. But if we try to evaluate `&a[i] < &b[j]` — two different
arrays — [the result is unspecified](http://eel.is/c++draft/expr.rel#4), which means it
can evaluate to whatever is most convenient for the compiler!

This is theoretically useful for optimizing compilers because it would let them transform

    static int arr[100];
    bool check_in_bounds(int *p) {
        return (arr <= p && p <= arr+100);
    }

into

    static int arr[100];
    bool check_in_bounds(int *p) {
        return true;
    }

since _either_ `(arr <= p && p <= arr+100)` is legitimately true
_or else_ the comparisons' behavior is unspecified and might as well yield `true`.

I have not observed any compiler performing this optimization in practice, but, thanks to
Krister Walfridsson's [blog post of
2016-12-15](https://kristerw.blogspot.com/2016/12/pointer-comparison-invalid-optimization.html),
I can see that GCC does have some interesting ideas about pointer comparisons.
[For example:](https://godbolt.org/z/M3PiSj)

    int x, y;
    int *p = &x + 1, *q = &y;

After these lines, GCC believes that `p < q`, `p == q`, and `p > q` are all `false`; but if you
ask about the disjunction of two of them — `(p < q || p == q)` — then GCC rewrites that into
`!(p > q)` and tells you the answer is `true`. So this is a very specific situation where
GCC believes that `(false || false) == true`.

----

Back to our snippet of Standardese.

> For templates `less`, `greater`, [et cetera], the specializations
> for any pointer type yield a strict total order [...] consistent with the partial order
> imposed by the built-in operators.

In other words, it is _not allowed_ for the compiler to believe that
`std::less<int*>()(p, q)`, `std::equal_to<int*>(p, q)`, and `std::greater<int*>(p, q)`
are all `false` — because then `p` and `q` would not be ordered relative to each other,
and so there wouldn't be a "strict total order" as required by the Standard.

[GCC gets the Standard behavior wrong](https://godbolt.org/z/VrJK5K), of course.
There's [an open libstdc++ bug](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78420) about this.

It looks to me as if Clang gets it right, basically by never trying to do GCC's "clever" optimization
on incomparable pointers.

(EDIT: Tim Song points out that the Standard does _not_ require `std::equal_to` to participate
in the strict total order, so, if two pointers are equal, then it is absolutely possible for
all of `less`, `equal_to`, and `greater` to be false, and [for all of `less_equal`, `greater_equal`
and `not_equal_to` to be true](https://godbolt.org/z/-ISHfq)! Oops. How could I be so blind?)

----

How does libc++ (the non-GNU library) implement `std::less<T*>` so as to give a strict total
order on pointers? [Here's the code.](https://github.com/llvm-mirror/libcxx/blob/7c3769df62c0b3820130aa868397a80a042e0232/include/__functional_base#L45-L55)

    template<class _Tp = void>
    struct less : binary_function<_Tp, _Tp, bool>
    {
        constexpr
        bool operator()(const _Tp& __x, const _Tp& __y) const
            {return __x < __y;}
    };

That's all. No magic. The libc++ authors basically assume that `<` Does The Right Thing for
pointers and provides a total order — and if it doesn't, well, [here's a nickel, get yourself
a better compiler](https://web.archive.org/web/20160319223530/http://2ndscale.com/rtomayko/2006/that-dilbert-cartoon).

(If you're just looking for best practices, you can stop here. The rest of this post is going
to be a horror story about what happens when you step off the path and head into the undergrowth.)

----

How does libstdc++ (the GNU library) implement `std::less<T*>` so as to try to give a strict
total order even in the presence of an adversarial optimizing compiler? Well,
[step one](https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/bits/stl_function.h#L379-L387)
is the same as libc++.
[Step two](https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/bits/stl_function.h#L428-L445)
is to make a partial specialization for pointer types.

    // Partial specialization of std::less for pointers.
    template<typename _Tp>
    struct less<_Tp*> : binary_function<_Tp*, _Tp*, bool>
    {
      constexpr bool
      operator()(_Tp* __x, _Tp* __y) const noexcept
      {
    #ifdef _GLIBCXX_HAVE_BUILTIN_IS_CONSTANT_EVALUATED
        if (__builtin_is_constant_evaluated())
    #else
        if (__builtin_constant_p(__x < __y))
    #endif
          return __x < __y;
        return (__UINTPTR_TYPE__)__x < (__UINTPTR_TYPE__)__y;
      }
    };

The idea here seems to be that _if_ the compiler can evaluate `__x < __y` as a constant expression,
then it should do so; and otherwise it should cast the operands to `uintptr_t` and do the comparison
that way.

The result of the comparison in `uintptr_t` is not guaranteed by the Standard to be
consistent with the result of the original comparison. (For example, let's say we're on a 64-bit platform
with 48 bits of real address space and 16 [tag bits](https://en.wikipedia.org/wiki/Tagged_pointer)
shoved into the high bits of the `uintptr_t`. For many more fun and exotic ways that casting to
`uintptr_t` might fail to preserve pointer ordering, see
Joe Nelson's ["C Portability Lessons from Weird Machines"](https://begriffs.com/posts/2018-11-15-c-portability.html) (2018-11-15).)
But that's fine. The libstdc++ authors basically
assume that casting-to-`uintptr_t` Does The Right Thing for pointers — and if it doesn't, well,
[here's a nickel, get yourself a better compiler](https://web.archive.org/web/20160319223530/http://2ndscale.com/rtomayko/2006/that-dilbert-cartoon).

The confusing thing about this code is that it seems to be doing the logic exactly backwards.
We're not worried about a _hardware platform_ where `<` does the wrong thing for pointers at
runtime! We're specifically worried about _the GCC compiler_, where `<` does the wrong thing
for pointers at compile-time. So delegating to the compiler's `<` at compile-time and the
hardware's cast-to-`uintptr_t` `<` at runtime seems exactly backwards, to me.
[Reversing the test makes the code behave as I would expect.](https://godbolt.org/z/iUfave)

Okay, so that's fixed it, right? All done? Unfortunately we're just getting started...

----

Consider [this snippet](https://godbolt.org/z/wm8hSJ):

    struct PtrHolder {
        int *p_;
        operator int*() const { return p_; }
    };

    bool foo() {
        int x, y;
        PtrHolder p{&x + 1}, q{&y};

        return (p < q);
    }

This code compiles! Since both `p` and `q` are implicitly convertible to `int*` (via the implicit
conversion operator), overload resolution chooses the built-in `<` operator for `int*`.
Therefore the behavior of this code is unspecified.

The behavior of `std::less<PtrHolder>()(p, q)` is also unspecified. ([Godbolt.](https://godbolt.org/z/24vrbv))

However:

> For template specializations `less<void>`, `greater<void>`, [et cetera], if the call operator
> calls a built-in operator comparing pointers, the call operator yields a strict total order [...]

This wording is a bit handwavey, since of course if "the call operator" (of `less<void>`) _actually_
calls a built-in operator comparing pointers, the result of that comparison will be unspecified
by definition. But basically this wording is trying to say that the behavior of
`std::less<void>()(p, q)` should be part of a strict total order!

[GCC gets this wrong too](https://godbolt.org/z/g85OIa)
([same bug](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78420)),
but let's look at how they attempt to do it.
[Here's the code.](https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/bits/stl_function.h#L576-L636)
I'll present a heavily massaged version:

    template<>
    struct less<void> {
        using is_transparent = void;

        template <class T, class U>
        constexpr auto operator()(T&& t, U&& u) const
            noexcept(noexcept(std::forward<T>(t) < std::forward<U>(u)))
            -> decltype(std::forward<T>(t) < std::forward<U>(u))
        {
            return do_compare(
                std::forward<T>(t), std::forward<U>(u),
                are_compared_by_builtin_pointer_comparison<T, U>{}
            );
        }

        // If the comparison doesn't use a built-in operator...
        template<class T, class U>
        static constexpr decltype(auto)
        do_compare(T&& t, U&& u, std::false_type) {
            return std::forward<T>(t) < std::forward<U>(u);
        }

        // If the comparison DOES use a built-in operator...
        template<class T, class U>
        static constexpr bool
        do_compare(T&& t, U&& u, std::true_type) {
            return std::less<const volatile void *>{}(
                static_cast<const volatile void*>( std::forward<T>(t) ),
                static_cast<const volatile void*>( std::forward<U>(u) )
            );
        }
    };

This first snippet presents the complete solution, except that we have not yet shown
the definition of `are_compared_by_builtin_operator<T, U>`. If the comparison *is* done
by a built-in operator, it must be because `t` and `u` are both implicitly convertible to
some pointer type, which means they must also be implicitly convertible to `const volatile void*`
(because every object pointer type is implicitly convertible to `const volatile void*`).
So we can use `std::less<const volatile void*>` to compare them — that's a solved problem
(except, as I showed, the logic actually committed in libstdc++ seems to be backwards).

So, how do we tell if the comparison of `t` and `u` is done via a built-in operator?
Well, this drags in another snippet of standardese. [[over.oper]/4](http://eel.is/c++draft/over.oper#4)
describes "operator functions," which is the formal name for overloaded operators:

> Operator functions are usually not called directly [...].
> They can be explicitly called, however, using the _operator-function-id_ as the name
> of the function in the function call syntax.
>
>     complex z = a.operator+(b);     // complex z = a+b;

Whereas the built-in operators are handled by different wording in
[[over.built]/1](http://eel.is/c++draft/over.built#1):

> The candidate operator functions that represent the built-in operators [...]
> are specified in this subclause. These candidate functions participate in the operator
> overload resolution process as described in [[over.match.oper]](http://eel.is/c++draft/over.match.oper)
> and are used for no other purpose.

Specifically, C++'s built-in operators _are not operator functions_ and cannot be
called using the explicit syntax! ([Godbolt.](https://godbolt.org/z/vB4CAF))

    std::complex<double> a, b;
    auto z = a.operator+(b);   // ERROR -- no such member function
    auto z = operator+(a, b);  // OK -- this free function exists

    double a, b;
    auto z = a.operator+(b);   // ERROR -- no such member function
    auto z = operator+(a, b);  // ERROR -- no such free function

Someone "cleverly" realized that we can use this quirk of the language to determine whether
a particular operator is built-in or not. If `t < u` compiles, but neither `t.operator<(u)`
nor `operator<(t, u)` compiles, then the comparison operator being used must be a built-in
operator! So we dive again into the weeds of libstdc++ (massaged greatly for presentation):

    template<class T, class U, class = void>
    struct are_compared_by_builtin_pointer_comparison : std::bool_constant<
        std::is_convertible_v<T, const volatile void*> &&
        std::is_convertible_v<U, const volatile void*>
    > {};

    // Throw out any types that use a member operator function.
    template<class T, class U>
    struct are_compared_by_builtin_pointer_comparison<T, U, decltype(void(
        std::declval<T>().operator<(std::declval<U>())
    ))> : std::false_type {};

    // Throw out any types that use an ADL free operator function.
    template<class T, class U>
    struct are_compared_by_builtin_pointer_comparison<T, U, decltype(void(
        operator<(std::declval<T>(), std::declval<U>())
    ))> : std::false_type {};

This "clever" logic does fail in at least one case.
Can you figure it out before reading the next section?

> "How often have I said to you that when you have eliminated the impossible, whatever remains,
> _however improbable_, must be the truth? We know that he did not come through the door,
> the window, or the chimney. We also know that he could not have been concealed in the room,
> as there is no concealment possible. Whence, then, did he come?"
>
> —Arthur Conan Doyle, [_The Sign of the Four_](https://www.pagebypagebooks.com/Arthur_Conan_Doyle/Sign_of_the_Four/Sherlock_Holmes_Gives_a_Demonstration_p2.html) (1890)

----

> "He came through the hole in the roof," I cried.
>
> —Arthur Conan Doyle, [_The Sign of the Four_](https://www.pagebypagebooks.com/Arthur_Conan_Doyle/Sign_of_the_Four/Sherlock_Holmes_Gives_a_Demonstration_p2.html) (1890)

Here's an example of a class type where `Alpha{} < Alpha{}` calls a
built-in comparison operator, but libstdc++'s clever metaprogramming will
not detect it.

    struct Alpha {
        struct Fake {};
        operator int*() const { return nullptr; }
        operator Fake() const { return Fake{}; }
    };
    template<class = void>
    bool operator<(Alpha::Fake, Alpha::Fake) { return true; }

    Alpha a;

Here the expression `operator<(a, a)` is _well-formed_,
so our clever metaprogramming will believe that an operator function exists and therefore
the built-in operator will not be used.

However, being _well-formed_ is not good enough — overload resolution will always select
the _best match_ according to [[over.match.best]](http://eel.is/c++draft/over.match.best#1.6),
where we find the following wording:

> a viable function `F1` is defined to be a better function than another viable function `F2` if
> [...] `F1` is not a function template specialization and `F2` is a function template specialization [...]

We have cleverly designed our `operator<` function so that it is a template specialization,
whereas the built-in candidate `bool operator<(int*, int*)` is _not_ a template specialization.
Therefore, even though the expression `operator<(a, a)` is well-formed, and the expression
`(a < a)` is well-formed, they have _different candidate sets_ and therefore _different resolutions_.
([Godbolt.](https://godbolt.org/z/YUKe5A))

    bool b1 = operator<(a, a); // true
    bool b2 = (a < a);         // false

I have verified that neither libstdc++'s "clever" metaprogramming, nor my simplified metaprogramming
shown in this blog post, is able to detect that `are_compared_by_builtin_pointer_comparison<Alpha, Alpha>`.
I suspect it is not possible to reliably detect this case.

----

> Sidenote for smarties: My massaged metaprogramming doesn't accept the following `struct Beta`, either,
> because of how I simplified the two partial specializations for presentation; but libstdc++'s
> actual un-massaged metaprogramming handles `Beta` just fine.
> 
>     struct Beta {
>         bool operator<(Beta) const { return false; }
>     };
>     template<class = void>
>     bool operator<(Beta, Beta) { return false; }

----

So there we are: into the weeds and back out, having learned for our trouble only that our
original problem — to detect when a "built-in operator comparing pointers is called" — is insoluble.

libc++ has it right: `std::less<void>` should be a one-liner. If you try to do more than that,
then not only are you solving a non-problem,
but your attempted solution will be [a source of bugs](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78420)
and even if you fix all the bugs, [your entire approach may still be wrong](https://godbolt.org/z/YUKe5A).

The moral of this horror story? [KISS!](https://en.wikipedia.org/wiki/KISS_principle)

----

P.S. — The unimplementable trait is [currently proposed as part of the Ranges TS](https://timsong-cpp.github.io/cppwp/ranges-ts/function.objects#comparisons-2)
under the name _`BUILTIN_PTR_CMP`_. Compiler vendors may choose to implement _`BUILTIN_PTR_CMP`_
by making it a compiler builtin. Or compiler vendors might choose the easier route (as Clang
apparently already does) and just make sure pointers are consistently totally ordered at the
native level. The latter would be a good thing, in my opinion.
