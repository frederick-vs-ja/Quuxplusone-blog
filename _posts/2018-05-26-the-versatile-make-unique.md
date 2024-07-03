---
layout: post
title: 'In praise of `make_unique`'
date: 2018-05-26 00:01:00 +0000
tags:
  c++-style
  smart-pointers
  standard-library-trivia
---

Lately I've been working on a codebase that does a lot of this:

    class Widget {
        using ptr = std::shared_ptr<Widget>;
        // ...
    };

    Widget::ptr p(new Widget(stuff));
    // ...
    p.reset(new Widget(otherstuff));
    // ...
    p.reset();

This style has advantages and disadvantages — but mostly, disadvantages.
The (lone?) advantage is that there is only a single mention of `std::` anywhere in
this code, which means that it's very easy to swap out `std::shared_ptr`
for `boost::shared_ptr` or any other smart pointer. (Actually, in my codebase,
the most likely candidate would be our hand-written smart pointer that does
intrusive ref-counting and uses RCU for reclamation.)

The other day, someone on Slack mentioned a disadvantage of this style:
they'd been using this style and accidentally wrote the last line above as

    p.release();  // oops, meant "p.reset"

This leaks the `shared_ptr` instead of freeing it. And compilers don't diagnose
this mistake (because `p.release();` is arguably a reasonable thing to write,
if you precede it with `q.take_ownership_of(p.get())`).

I claim that the problem of "misspelling the name of an obscure mutator method"
goes away entirely if you make a habit of _not using obscure mutator methods._
So, I refactor the `Widget` snippet this way:

    class Widget {
        // ...
    };

    auto p = std::make_shared<Widget>(stuff);
    // ...
    p = std::make_shared<Widget>(otherstuff);
    // ...
    p = nullptr;

This style loses the ability to switch to `boost::shared_ptr` with a one-line patch,
but it gains several other advantages:

- No raw `new` and `delete`. I try to make it so that `git grep -l 'new '`
  returns no results, or at least returns a small O(1) number of results
  (for example, a single use of placement `new` syntax deep in the guts of
  some utility type). If your code is littered with `.reset(new ...)`, then
  you'll never be able to find that one memory leak caused by an unmanaged `new`.

- Fewer allocations, because of [how `make_shared` works](https://stackoverflow.com/questions/20895648/).

- More readable by novice programmers; you might have to look up what `reset`
  means, but `operator=` is immediately intuitive (to curly-brace programmers,
  anyway).

- No chance of typo'ing `reset` as `release`.

And of course the same thing goes for `std::unique_ptr` and
[`std::make_unique`](http://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique),
which is why it's so nice that we finally got `std::make_unique` in C++14.


## The versatile `unique_ptr`

One little-known but very useful fact about `std::unique_ptr` and `std::shared_ptr`
is that the former *implicitly converts* to the latter!

    template<class Y, class Deleter>
    shared_ptr(std::unique_ptr<Y, Deleter>&& r);

So if we have a codebase that mixes `shared_ptr` and `unique_ptr` (perhaps we're
in the process of converting the former to the latter wherever possible), it is
convenient that

    myptr_ = std::make_unique<T>();

always compiles, regardless of whether `myptr_` is a `std::shared_ptr<T>`
or a `std::unique_ptr<T>`.


## Should `make_widget` return `unique_ptr`?

This means that if you have a factory function that returns a new object, you
*may* be better off making it return `unique_ptr` than `shared_ptr`, even if you
plan usually to use the result as a `shared_ptr`!
Here are some common usage patterns for such a factory function:

    std::shared_ptr<Widget> make_widget() {
        return std::make_shared<WidgetImpl>(stuff);
    }

    class WidgetTestHarness {
        std::shared_ptr<Widget> widget_;
        WidgetTestHarness() {
            widget_ = make_widget();
        }
    };

    void view_widget(Widget& w);
    void keep_widget(std::shared_ptr<Widget> sptr);

    void manipulator() {
        auto w = make_widget();
        keep_widget(w);
        view_widget(*w);
    }

Notice (well, suppose) that our `WidgetTestHarness` never needs to copy its
`shared_ptr<Widget>`. Really it just wants a member `Widget widget_;`, but it
can't do that because the only way to create a `Widget` object is via `make_widget`.
(And notice why that is: `make_widget()` secretly creates a `WidgetImpl` object,
not a base `Widget`!)  So it would settle for holding a `unique_ptr<Widget>`,
if it could.

Philosophically, it makes sense that `make_widget()` should return a `unique_ptr`.
I mean, it's creating a *brand-new* `Widget`. There's no way that anyone else in
the program could already hold a reference to this `Widget`. So the pointer returned
from `make_widget` is, literally, a *unique* pointer. We are certainly tempted to
represent that unique pointer using `unique_ptr`.

Let's see if we can make that happen.

    std::unique_ptr<Widget> make_widget() {
        return std::make_unique<WidgetImpl>(stuff);  // Danger, Will Robinson!
    }

    class WidgetTestHarness {
        std::unique_ptr<Widget> widget_;
        WidgetTestHarness() {
            widget_ = make_widget();
        }
    };

    void view_widget(Widget& w);
    void keep_widget(std::shared_ptr<Widget> sptr);

    void manipulator() {
        std::shared_ptr<Widget> w = make_widget();  // Slightly awkward.
        keep_widget(w);
        view_widget(*w);
    }

This rewrite gets us the behavior we want — `unique_ptr` in `WidgetTestHarness` — with
*relatively* little downside. I've thought of two downsides, annotated above.

Downside number one: When we convert `unique_ptr<WidgetImpl>` to `unique_ptr<Widget>`,
we change the deleter as well, from `default_delete<WidgetImpl>` (which calls `~WidgetImpl`)
to `default_delete<Widget>` (which calls `~Widget`). If `~Widget` is not `virtual`, then
we have introduced a bug. So we'd better make dang sure that our `Widget` has a virtual
destructor, if we're going to be using this pattern with `make_widget`.

I could swear that *sometimes* Clang is smart enough to diagnose the accidental call to
`Widget`'s non-virtual destructor, but [not in this case](https://wandbox.org/permlink/7brZrTZfRUqIY1JB).
(GCC also does not diagnose it, but I wouldn't expect them to.)

Downside number two: If we keep the `auto w` in `manipulator()`, then the call to
`keep_widget(w)` won't compile. You can implicitly convert an _rvalue_ `unique_ptr` to
`shared_ptr` (the `shared_ptr` just steals ownership from the `unique_ptr`),
but you can't convert an _lvalue_ `unique_ptr` to `shared_ptr` (because if we stole the
ownership from `w`, then the following line's `view_widget(*w)` would blow up with a null
pointer dereference).

Therefore, we had to replace `auto w` with `std::shared_ptr<Widget> w`, which
might be too much typing for some people (and makes the patch even bigger if we ever want to
switch to `boost::shared_ptr`).

Notice that in C++17 we can write simply

    std::shared_ptr w = make_widget();

which is still a lot of typing, and incidentally I'm not a fan of gratuitous CTAD,
*and* incidentally this line won't work on libc++ yet because they have not implemented
[deduction guides for `shared_ptr`](http://en.cppreference.com/w/cpp/memory/shared_ptr/deduction_guides) yet.
But libstdc++ has implemented them; and presumably libc++ will follow suit relatively soon.
(C++17 deduction guides for the STL containers are
[landing](https://github.com/llvm-mirror/libcxx/commit/32bc2e298c6fa521) in libc++ as we speak!)
