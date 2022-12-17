---
layout: post
title: 'Left-folding and right-folding an arbitrary callable'
date: 2020-10-12 00:01:00 +0000
tags:
  metaprogramming
  cpplang-slack
  variadic-templates
---

Nerdsnipe: Given an arbitrary binary callable `f(x,y)`, write a function `left_fold` such that `left_fold(f)(a,b,c,...)`
evaluates to `f(f(f(a,b),c),...)`.  Write a function `right_fold` such that `right_fold(f)(a,b,c,...)` evaluates to
`f(a, f(b, f(c, ...)))`.

That is, we're looking to define `left_fold` and `right_fold` such that

    auto leftsub = left_fold(std::minus<>);
    auto rightsub = right_fold(std::minus<>);
    static_assert(leftsub(1, 2, 3) == (1 - 2) - 3);
    static_assert(rightsub(1, 2, 3) == 1 - (2 - 3));

This is easy with "recursive templates," but in modern C++ we know that
["Iteration is better than recursion"](/blog/2018/07/23/metafilter/) (2018-07-23), so let's try to
do it with C++17 fold-expressions. [Godbolt](https://godbolt.org/z/K8s8qT):

    #define FWD(x) static_cast<decltype(x)>(x)
    #define MOVE(x) static_cast<decltype(x)&&>(x)

    template<class F, class TRR>
    struct Folder {
        const F& foo_;
        TRR value_;

        template<class U>
        constexpr auto operator+(Folder<F, U>&& rhs) && -> decltype(auto) {
            using R = decltype(foo_(FWD(value_), FWD(rhs.value_)));
            return Folder<F, R>{foo_, foo_(FWD(value_), FWD(rhs.value_))};
        }
    };

    template<class F>
    constexpr auto left_fold(F foo) {
        return [foo = MOVE(foo)](auto&&... args) -> decltype(auto) {
            return (... + Folder<F, decltype(args)>{foo, FWD(args)}).value_;
        };
    }

    template<class F>
    constexpr auto right_fold(F foo) {
        return [foo = MOVE(foo)](auto&&... args) -> decltype(auto) {
            return (Folder<F, decltype(args)>{foo, FWD(args)} + ...).value_;
        };
    }

There are a couple of tricks hiding in here:

* The `FWD(x)` macro is just a shorter spelling of `std::forward<X>(x)`.
    I normally use `static_cast<X&&>(x)`, but in this case I have several
    places where the type `X` is not easily nameable.

* The `MOVE(x)` macro is just a shorter spelling of `std::move(x)`, and
    I'm using it here only to capture `foo` by move without having to include
    any standard library headers. I want to capture `foo` by move, just in case
    it's a move-only lambda type.

* All of the uses of `-> decltype(auto)` are necessary. Try removing them
    and seeing what goes wrong in folding `<<` over `(std::cout, 1, 2, 3)`.

* We use aggregate initialization for `Folder<F, R>{...}`, so that the
    value of `foo_(FWD(value_), FWD(rhs.value_))` will be constructed in-place
    rather than constructing a temporary and then having to move it into place.
    This allows us to work with non-move-constructible types.

However, if the overall result of the fold is a prvalue of a
non-move-constructible type, we fail ([Godbolt](https://godbolt.org/z/bcqzeT)).
I can partially fix `left_fold`'s issue with non-move-constructible types,
but (1) I cannot fix `right_fold`; (2) I cannot _totally_ fix `left_fold`;
and (3) my change to `left_fold` causes additional failures in even more
pathological cases. ([Godbolt.](https://godbolt.org/z/E19j6r))

    template<class F, class TRR>
    struct LeftFolder {
        const F& foo_;
        TRR value_;

        template<class U>
        constexpr friend auto operator+(U&& lhs, LeftFolder&& rhs) -> decltype(auto) {
            return rhs.foo_(FWD(lhs), FWD(rhs.value_));
        }
    };

    template<class F>
    constexpr auto left_fold(F foo) {
        return [foo = MOVE(foo)](auto&& init, auto&&... args) -> decltype(auto) {
            return (FWD(init) + ... + LeftFolder<F, decltype(args)>{foo, FWD(args)});
        };
    }
