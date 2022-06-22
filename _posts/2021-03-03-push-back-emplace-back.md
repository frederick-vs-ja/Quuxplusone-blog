---
layout: post
title: "Don't blindly prefer `emplace_back` to `push_back`"
date: 2021-03-03 00:01:00 +0000
tags:
  benchmarks
  c++-learner-track
  compile-time-performance
  move-semantics
  pitfalls
---

In one of my recent training courses, a student informed me that both clang-tidy
and PVS-Studio were complaining about some code of the form

    std::vector<Widget> widgets;
    ~~~
    widgets.push_back(Widget(foo, bar, baz));

Both tools flagged this line as "bad style."
clang-tidy even offered a (SARCASM ALERT) helpful fixit:

    warning: use emplace_back instead of push_back [modernize-use-emplace]
        widgets.push_back(Widget(foo, bar, baz));
                ^~~~~~~~~~~~~~~~~             ~
                emplace_back(

The student dutifully changed the line, and both tools reported their
satisfaction with the replacement:

    widgets.emplace_back(Widget(foo, bar, baz));

The original line materializes a temporary `Widget` object on the stack;
takes an rvalue reference to it; and passes that reference to
`vector<Widget>::push_back(Widget&&)`, which move-constructs a `Widget`
into the vector. Then we destroy the temporary.

The student's replacement materializes a temporary `Widget` object on the stack;
takes an rvalue reference to it; and passes that reference to
`vector<Widget>::emplace_back<Widget>(Widget&&)`, which move-constructs
a `Widget` into the vector. Then we destroy the temporary.

_Absolutely no difference._

The change clang-tidy meant to suggest — and in fact _did_ suggest,
if you pay very close attention to the underlining in the fixit — was actually this:

    widgets.emplace_back(foo, bar, baz);

This version does _not_ materialize any `Widget` temporaries. It simply
passes `foo, bar, baz` to `vector<Widget>::emplace_back<Foo&, Bar&, Baz&>(Foo&, Bar&, Baz&)`,
which constructs a `Widget` into the vector using whatever
constructor of `Widget` best matches that bunch of arguments.


## `emplace_back` is not magic C++11 pixie dust

Even a decade after C++11 was released, I still sometimes see programmers assume
that `emplace_back` is somehow related to move semantics. (In the same way that
some programmers assume lambdas are somehow the same thing as `std::function`,
you know?) For example, they'll rightly observe that this code makes an
unnecessary copy:

    void example() {
        auto w = Widget(1,2,3);
        widgets.push_back(w);  // Copy-constructor alert!
    }

So they'll change it to this:

    void example() {
        auto w = Widget(1,2,3);
        widgets.emplace_back(w);  // Fixed? Nope!
    }

The original line constructs a `Widget` object into `w`, then
passes `w` by reference to `vector<Widget>::push_back(const Widget&)`,
which copy-constructs a `Widget` into the vector.

The replacement constructs a `Widget` object into `w`, then
passes `w` by reference to `vector<Widget>::emplace_back<Widget&>(Widget&)`,
which copy-constructs a `Widget` into the vector.

_Absolutely no difference._

What the student should have done is ask the compiler to make an
_rvalue_ reference to `w`, by saying either

    widgets.push_back(std::move(w));

or

    widgets.emplace_back(std::move(w));

It doesn't matter which verb you use; what matters is the value category of
`w`. You must explicitly mention `std::move`, so that the language (and the
human reader) understand that you're done using `w` and it's okay for
`widgets` to pilfer its guts.

`emplace_back` was added to the language at the same time as `std::move` — just
like lambdas were added at the same time as `std::function` — but that doesn't
make them the same thing. `emplace_back` may "look more C++11-ish," but it's
not magic move-enabling pixie dust and it will never insert a move in a place
you don't explicitly request one.


## When all else is equal, prefer `push_back` to `emplace_back`

So, given that these two lines do the same thing and are equally efficient
at runtime, which should I prefer, stylistically?

    widgets.push_back(std::move(w));
    widgets.emplace_back(std::move(w));

I recommend sticking with `push_back` for day-to-day use. You should definitely
use `emplace_back` when you need its particular set of skills — for example, `emplace_back`
is your only option when dealing with a `deque<mutex>` or other non-movable type —
but `push_back` is the appropriate default.

One reason is that `emplace_back` is more work for the compiler.
`push_back` is an overload set of two non-template member functions.
`emplace_back` is a single variadic template.

    void push_back(const Widget&);
    void push_back(Widget&&);

    template<class... Ts>
    reference emplace_back(Ts&&...);

When you call `push_back`, the compiler must do overload resolution, but that's all.
When you call `emplace_back`, the compiler must do template type deduction, followed
by (easy-peasy) overload resolution, followed by function template instantiation and
code generation. That's a much larger amount of work for the compiler.


## The benchmark program

I wrote a simple test program to demonstrate the difference in compiler workload.
Of course [Amdahl's Law](https://en.wikipedia.org/wiki/Amdahl%27s_law) applies:
my benchmark displays a massive difference because it's doing _nothing but_
instantiating `emplace_back`, whereas any production codebase will be doing vastly
more other stuff relative to the number of times it instantiates `emplace_back`.
Still, I hope this benchmark gives you a sense of why I recommend "`push_back` over
`emplace_back`" and not vice versa.

This Python 3 script generates the benchmark:

    import sys
    print('#include <vector>')
    print('#include <string>')
    print('extern std::vector<std::string> v;')
    for i in range(1000):
        print('void test%d() {' % i)
        print('    v.%s_back("%s");' % (sys.argv[1], 'A' * i))
        print('}')

Generate like this:

    python generate.py push >push.cpp
    python generate.py emplace >emplace.cpp
    time g++ -c push.cpp
    time g++ -c emplace.cpp

With Clang trunk on my laptop, I get consistently about 1.0s for the `push` version,
and 4.2s for the `emplace` version. This big difference is due to the fact that the
`push` version is merely code-generating a thousand `test` functions, whereas
the `emplace` version is code-generating that same thousand `test` functions *and*
another thousand template instantiations of `emplace_back` with different parameter
types:

    vector<string>::emplace_back<const char(&)[1]>(const char (&)[1])
    vector<string>::emplace_back<const char(&)[2]>(const char (&)[2])
    vector<string>::emplace_back<const char(&)[3]>(const char (&)[3])
    vector<string>::emplace_back<const char(&)[4]>(const char (&)[4])
    ~~~

See, `push_back` knows that it expects a `string&&`, and so it knows to call the
non-explicit constructor `string(const char *)` on the caller's side. The same
constructor is called in each case, and the temporary `string` is passed to
the same overload of `push_back` in each case. `emplace_back`, on the other hand,
is a dumb perfect-forwarding template: it doesn't know that the relevant constructor
overload will end up being `string(const char *)` in each case. So it takes
an lvalue reference to the specific _array type_ being passed by the caller.
Perfect-forwarding has no special cases for `const char *`!

If we change `vector<string>` to `vector<const char *>`, the compile-time-performance
gap widens: now it's 0.7s for `push`, 3.8s for `emplace`. This is because we've cut
out some of the work that was common to both versions (constructing `std::string` objects)
without affecting the source of the gap (that one version instantiates a
thousand copies of `emplace_back` and the other doesn't). Amdahl's Law in action!

My conclusions:

> Use `push_back` by default.

> Use `emplace_back` where it is semantically significant to your algorithm
> (such as when the element type's move-constructor is absent or has been
> benchmarked as expensive).

> Avoid mixing string literals and perfect-forwarding templates,
> especially in repetitive machine-generated code.

----

Previously on this blog:

* ["The surprisingly high cost of static-lifetime constructors"](/blog/2018/06/26/cost-of-static-lifetime-constructors/) (2018-06-26)
