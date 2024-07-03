---
layout: post
title: "PSA: `shared_ptr<T>()` is not `make_shared<T>()`"
date: 2019-03-06 00:01:00 +0000
tags:
  pitfalls
  smart-pointers
  war-stories
---

The other day, while working on the same circa-2012 codebase that generated
[my previous blog post on `shared_ptr(&w, _1)`](/blog/2019/02/06/boost-lambda-abuse) (2019-02-06),
I noticed something perhaps noteworthy. When you have code of the form

    Widget::ptr widget(new Widget);

you often want to modernize it into

    std::shared_ptr<Widget> widget(new Widget);

and then into

    auto widget = std::make_shared<Widget>();

(which, notice, [is subtly different](https://stackoverflow.com/a/20895705/1424877)
in a way that doesn't matter to almost any codebase: `make_shared` will merge the
allocations of the Widget and its control block, which has good ramifications for
memory usage in general and bad ramifications for memory usage if you expect to
have a lot of expired `weak_ptr`s in your program).

However, if you are masochistic enough to do this kind of transformation *manually*,
you must be very careful that you don't brain-fart it into

    auto widget = std::shared_ptr<Widget>();

The above is well-formed and valid C++ code. It gives `widget` the exact same static type.
The only difference is that instead of setting `widget` to a new heap-allocated `Widget`,
it sets `widget` to `nullptr`!

The moral of the story is to be careful when modernizing. But the good news is that if
all your code follows the same known style — if code that *does* the same thing,
always *looks* the same — then it is usually pretty easy to grep for anomalies.

For example, the only place that `std::shared_ptr<.*>(.*)` appears in our newly modernized
codebase is in a couple of places that use the aliasing constructor, and unfortunately
in our unit-test code, where [GMock](https://github.com/google/googletest/blob/master/googlemock/docs/CheatSheet.md)
requires us to write things like

    class MockWidgetFactory : public WidgetFactory {
        MOCK_METHOD0(produceWidget, std::shared_ptr<Widget>());
    };
