---
layout: post
title: "`shared_ptr(&w, _1)`"
date: 2019-02-06 00:01:00 +0000
tags:
  smart-pointers
  war-stories
---

Today I saw some code (from circa 2012) that looked like this:

    Widget::ptr 
    Widget::instance() {
        static Widget w;
        static Widget::ptr p(&w, boost::lambda::_1);
        return p;
    }

As is relatively common in "early modern" C++ code, `Widget::ptr` is a
typedef for `boost::shared_ptr<Widget>`.

My first thought was that this was a broken attempt to make a `shared_ptr`
that called `w` when the last copy went out of scope. ("Broken," because `p` itself has
static storage duration and so the last copy won't go out of scope until the very
end of the program.) I had to put the code into [Godbolt Compiler Explorer](https://godbolt.org/z/2Znrgm) to figure
out what was actually happening.

----

For those readers who haven't seen [Boost.Lambda](https://theboostcpplibraries.com/boost.lambda) before,
it's a bunch of template metaprogramming with the ultimate effect that you can write something like

    using namespace boost::lambda;

    std::copy_if(
        src.begin(), src.end(), dst.begin(),
        (minVal < _1) || (_1 < maxVal)
    );

    std::for_each(
        v.begin(), v.end(),
        std::cout << _1 << "\n"
    );

as opposed to the pre-modern approach of writing your own functor class from scratch, or the modern approach of writing

    std::copy_if(
        src.begin(), src.end(), dst.begin(),
        [](const auto& x) { return (minVal < x) || (x < maxVal); }
    );

    std::for_each(
        v.begin(), v.end(),
        [](const auto& x) { std::cout << x << "\n"; }
    );

(Could I have written `auto&& x` in both cases to achieve the same effect?
[I sure could.](/blog/2018/12/15/autorefref-always-works/))

----

So, if `boost::lambda::_1 + 1` is a Boost Lambda function with the meaning "Take whatever arguments
you receive and return the first one plus 1," then `boost::lambda::_1` on its own is a Boost Lambda
function with the meaning "Take whatever arguments you receive and return the first one with no
modification."

In other words, this is a function with no side effects at all.

So our original code snippet:

    Widget::ptr 
    Widget::instance() {
        static Widget w;
        static Widget::ptr p(&w, boost::lambda::_1);
        return p;
    }

is equivalent to

    Widget::ptr 
    Widget::instance() {
        static Widget w;
        static Widget::ptr p(&w, [](auto&& p) { return p; });
        return p;
    }

which is equivalent in this context to the following snippet — the snippet
with which I replaced the original code, eliminating the Boost.Lambda dependency —

    Widget::ptr 
    Widget::instance() {
        static Widget w;
        static Widget::ptr p(&w, [](auto *) {});
        return p;
    }

That is, the original snippet is a _perfectly correct_ (but "clever") way
of writing a function that gives back a pointer to the `w` singleton instance,
_as a smart pointer_, but a smart pointer whose deleter does nothing.
(Which is as it should be, because we never want to `delete w`.)

TLDR: `boost::lambda::_1` is an "early modern C++" idiom meaning "a no-op lambda."
