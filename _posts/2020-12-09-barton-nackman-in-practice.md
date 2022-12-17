---
layout: post
title: "An example of the Barton–Nackman trick"
date: 2020-12-09 00:01:00 +0000
tags:
  argument-dependent-lookup
  hidden-friend-idiom
  cpplang-slack
  stl-classic
  templates
---

I still need to write the blog post explaining "What is the Hidden Friend Idiom?";
but for now, here's a practical demonstration of its usefulness that just came up
on the cpplang Slack (thanks, Johan Lundberg!).

Consider this pair of class templates `Cat<T>` and `Dog<T>`. `Cat` is analogous to
the STL's container templates, which define `operator<` (or in C++20, `operator<=>`)
as a free function template.

    template<class V>
    struct Cat {
        V value_;
    };

    template<class V>
    bool operator<(const Cat<V>& a, const Cat<V>& b) {
        return a.value_ < b.value_;
    }

`Dog` is how I _recommend_ everyone write their
overloaded operators, as hidden friend non-templates.

    template<class V>
    struct Dog {
        V value_;

        friend bool operator<(const Dog& a, const Dog& b) {
            return a.value_ < b.value_;
        }
    };

Now consider the following "`sort_in_place`" algorithm, which
uses `reference_wrapper` to sort a vector of _handles pointing into_
its const argument vector, instead of sorting the argument vector
itself. (I've never found this trick useful in practice, but it's
a very cute demonstration of how the different pieces of the STL
fit together. I use basically this code in the `reference_wrapper`
unit of my ["Classic STL"](https://cppcon.org/class-2020-classic-stl/) training course.)

    template<class T>
    void sort_in_place(const std::vector<T>& vt) {
        std::vector<std::reference_wrapper<const T>> vr(vt.begin(), vt.end());
        std::sort(vr.begin(), vr.end());
        std::transform(vr.begin(), vr.end(),
            std::ostream_iterator<int>(std::cout), std::mem_fn(&T::value_));
    }

([Godbolt.](https://godbolt.org/z/j74PsfK3z))

We observe that `sort_in_place` works nicely for `Dog`. Whenever `std::sort`
needs to compute `a < b`, where `a` and `b` are `reference_wrapper<Dog<int>>`,
it uses ADL to find our friend as a candidate, and then confirms that `reference_wrapper<Dog<int>>`
is indeed implicitly convertible to `const Dog<int>&` for both arguments.

But `sort_in_place` fails for `Cat`! It still uses ADL to find our `operator<`
template; but then template argument deduction requires that the provided argument types
exactly match the pattern specified by the template, which is not true in this case —
there is no `V` such that `Cat<V>` is `reference_wrapper<Cat<int>>`. Deduction fails.

We can manually call the proper specialization — the one with `V=int` — by writing
`operator< <int>(vr[0], vr[1])`. (The space is important there!)
But we can't write simply `vr[0] < vr[1]` because of this poor interaction with
template argument deduction.

So `Dog`'s `operator<` is better-designed than `Cat`'s, at least in this respect.


## Conclusion

> For operator overloads, always prefer "hidden friend" non-templates.

The _main_ benefits of the hidden friend idiom are that it avoids member operators'
asymmetrical treatment of the left and right operands, and that it shrinks overload sets
by not dumping free operators directly into the top-level namespace. This blog post
didn't relate to either of those benefits.

The use of the hidden friend idiom in cases like `Dog` — specifically to create a
class template whose instantiations are associated with _non-template_ friends —
is known as the "Barton–Nackman trick." This blog post showed one way in which
the Barton–Nackman trick, specifically, provides a subtle benefit.

Even in C++20, STL containers generally use free templates instead of the Barton–Nackman trick,
which means that `sort_in_place` will, for example, _work_ when `vt` is `vector<int>`
but _fail_ when `vt` is `vector<string>`. ([Godbolt.](https://godbolt.org/z/T57z8co6f))

----

See also:

* ["`operator<=>` doesn't obsolete the hidden friend idiom"](/blog/2021/10/22/hidden-friend-outlives-spaceship/) (2021-10-22)
