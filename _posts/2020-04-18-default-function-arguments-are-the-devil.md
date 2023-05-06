---
layout: post
title: 'Default function arguments are the devil'
date: 2020-04-18 00:01:00 +0000
tags:
  c++-learner-track
  c++-style
  language-design
  library-design
  rant
  slogans
excerpt: |
  If you frequently talk with me about C++, you have undoubtedly heard me say:

  > Default function arguments are the devil.

  My position opposes the historical (early-1990s) style of the STL, of course; but I was recently
  alerted that it's also in direct conflict with the (2010s) C++ Core Guidelines, specifically
  [rule F.51](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-default-args).
  Since this conflict might, if unaddressed, lead people to think I'm _wrong_, I'd better address it. ;)
---

If you frequently talk with me about C++, you have undoubtedly heard me say:

> Default function arguments are the devil.

My position opposes the historical (early-1990s) style of the STL, of course; but I was recently
alerted that it's also in direct conflict with the (2010s) C++ Core Guidelines, specifically
[rule F.51](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-default-args).
Since this conflict might, if unaddressed, lead people to think I'm _wrong_, I'd better address it. ;)

I'll present a short hypothesis on the evolution of default function arguments as a language feature
(why do they exist? what problems did they solve in the 1990s?), and then present all the ways
I can think of for how they cause problems.


## Why do default function arguments exist?

Even the language's creator considered default function arguments to be old-fashioned... 26 years ago!

> Given general function overloading, default arguments are logically redundant and at best a
> minor notational convenience. However, C with Classes had default argument lists for years
> before general overloading became available in C++.
>
> —Bjarne Stroustrup, _[D&E](https://amzn.to/2yli2Yp)_ (1994), section 2.12.2

However, I think there are at least two reasons that the default-function-argument style
did not go extinct alongside "C with Classes" but rather survived into the late 1990s and beyond:

- Compilers in the 1990s were bad at inlining.

- C++98 did not have delegating constructors.

First, consider:

    int nexta(int x, int y);
    inline int nexta(int x) { return nexta(x, 1); }

    int nextb(int x, int y = 1);

Since compilers' inlining heuristics were bad, there was a very real possibility that a call to `nexta(42)`
would end up executing _two_ `call` instructions, pushing _two_ stack frames, and so on. Also, the compiler
might choose to generate out-of-line code for `nexta(int)` and thus bloat your executable size — more worrisome
on that decade's smaller machines. In this decade, even on smaller machines, we have smarter compilers:
I do not worry that simple inline functions like this would cause noticeably bigger codegen.

Second, consider that if you wanted to write an overload set of constructors in C++98, you'd have to do
something like this:

    // Much repeated initialization code
    struct Widget {
        std::string data_;

        explicit Widget(int size) : data_(size) {
            std::iota(data_.begin(), data_.end(), 'A');
        }
        explicit Widget(int size, char start) : data_(size) {
            std::iota(data_.begin(), data_.end(), start);
        }
    };

and the only way to factor out the repeated code (without default function arguments)
would be to move to a domesticated form of "two-phase initialization," like this:

    // Two-phase initialization
    struct Widget {
        std::string data_;

        explicit Widget(int size) { init(size, 'A'); }
        explicit Widget(int size, char start) { init(size, start); }
    private:
        void init(int size, int start) {
            data_ = std::string(size);
            std::iota(data_.begin(), data_.end(), start);
        }
    };

(This solution _additionally_ suffers from our first problem: poor inlining heuristics in old compilers.)
C++98's solution was to use default function arguments to eliminate
repeated code without relying on two-phase initialization.

    // C++98's solution: default function arguments
    struct Widget {
        std::string data_;

        explicit Widget(int size, char start = 'A') : data_(size) {
            std::iota(data_.begin(), data_.end(), start);
        }
    };

But in C++11 and later, we have _delegating constructors._ We can write this code straightforwardly,
turning `init` back into a proper constructor. Compare the following C++11 code with the two-phase
initialization code from above.

    // C++11's solution: delegating constructors
    struct Widget {
        std::string data_;

        explicit Widget(int size) : Widget(size, 'A') {}
        explicit Widget(int size, int start) : data_(size) {
            std::iota(data_.begin(), data_.end(), start);
        }
    };

With today's delegating constructors and good inliners, there's no longer any _technical_ reason
to use default function arguments. If we were designing C++ from scratch today, we probably wouldn't
feel any need to include them at all.

Now for the various downsides of default function arguments.
As Wikipedia says, "This is a dynamic list. You can help by adding to it."


## Default function arguments produce convoluted overload sets

Here's my go-to example, which is actually extracted from some teaching material that _recommends_ the use of
default function arguments! Our goal is to write a `print_square` function that prints an `n`-by-`n`
square filled with a specific ASCII character. If `n` is missing, default it to 10. If `fill` is missing,
default it to `+`.

Students often try to mechanically translate this English description into C++ code, like this:

    // square.h
    void print_square(int n = 10, char fill = '+');

    // square.cpp
    void print_square(int n, char fill) {
        auto row = std::string(n, fill);
        for (int i=0; i < n; ++i)
            std::cout << row << "\n";
    }

Then they run the unit tests:

    print_square(5, '*');  // OK
    print_square(5);       // OK, acts like (5, '+')
    print_square();        // OK, acts like (10, '+')
    print_square('*');     // Oh no!!

The fourth line above acts like `print_square('*', '+')` — it prints a 42-by-42 square of `+` characters!
We could "easily" fix this bug without giving up our default function arguments, by adding an additional overload:

    // square.h
    void print_square(int n = 10, char fill = '+');
    inline void print_square(char fill) { return print_square(10, fill); }

Or alternatively:

    // square.h
    void print_square(int n, char fill = '+');
    inline void print_square(char fill = '+') { return print_square(10, fill); }

However, I assert that neither of these solutions is _clear_.
Making any kind of modification to the signatures in this overload set will require very careful auditing,
to make sure you're not introducing an ambiguity (one signature with two possible candidates) or accepting
a signature that shouldn't be accepted (one candidate with multiple signatures, some of which are unwanted).
We are creating a puzzle for our reader... and for ourselves.

> Do the two alternative solutions above provide exactly the same set of signatures?
> How long did it take to convince yourself of your answer?

The most maintainable, least "puzzley" approach is simply to provide one overload per signature,
each signature on a line by itself.

    // square.h
    void print_square(int n, char fill);
    inline void print_square(int n) { print_square(n, '+'); }
    inline void print_square(char fill) { print_square(10, fill); }
    inline void print_square() { print_square(10, '+'); }

This code is trivially correct by inspection, and therefore easy to maintain and extend.


## One name, varied functionalities

Nir Friedman points out that our refactoring of `print_square` shouldn't stop there.
We have succeeded in unpuzzling our API from the _maintainer's_ point of view; but
now let's consider the puzzle from the _client's_ point of view. The client programmer
doesn't want a "puzzling" `print_square(x)` that treats `x` sometimes as a side length
and sometimes as a fill character! The client programmer wants an API that looks more
like this:

    // square.h
    void print_square(int n, char fill);
    inline void print_square_with_side(int n) { print_square(n, '+'); }
    inline void print_square_with_fill(char fill) { print_square(10, fill); }
    inline void print_default_square() { print_square(10, '+'); }

In fact, maybe the client programmer doesn't even need some of those functionalities
at all, and we can just eliminate those unused functions from our code.
See ["Refactoring with `=delete`"](/blog/2022/11/11/refactoring-with-delete/) (2022-11-11)
and ["API design advice from Anakin and Obi-Wan"](/blog/2022/12/17/kill-three-member-functions/) (2022-12-17).

Reusing the same name for multiple distinct functionalities is a trap. It's still
possible to fall into that trap without using default arguments. But it isn't possible
to use default arguments without falling into that trap. The next section shows
another example:


## The "boolean parameter tarpit"

We've all seen code like this, right?

    void doTask(Task task, std::seconds timeout, bool detach = false,
                bool captureStdout = false, bool captureStderr = false);

    doTask(task1, 100s);
    doTask(task1, 3600s, true);
    doTask(task2, 3600s, false, true, false);
    doTask(task2, 6000s, true, true);
    doTask(task1, 200s, false, true, true);

Refactoring this code to use "one overload per signature" won't magically make it good code.
But as we refactor, we may find some bugs anyway:

    void doTask(Task task, std::seconds timeout);
    void doTask(Task task, std::seconds timeout, bool detach);
    void doTask(Task task, std::seconds timeout, bool detach,
                bool captureStdout, bool captureStderr);

    doTask(task2, 6000, true, true);  // ERROR: no longer compiles!

Did the original programmer of this line _know_ that this was performing the equivalent of
`doTask(task2, 6000, true, true, false)`? Or did they accidentally omit the `detach` parameter?

Orthogonally, suppose we leave the default function arguments in place, and then we get a feature
request to add another parameter: `bool shell = false`. But it turns out that everyone who calls
`doTask(task1)` really needs that parameter to be `true`. How easy is the refactor?

|:------------------------------------------|:--------------------------------------------------|
| `doTask(task1, 100s);`                    | `doTask(task1, 100s, false, false, false, true);` |
| `doTask(task1, 3600s, true);`             | `doTask(task1, 3600s, true, false, false, true);` |
| `doTask(task1, 200s, false, true, true);` | `doTask(task1, 200s, false, true, true, true);`   |
{:.smaller}

Adding one parameter to the very end of a call requires spelling out all of the preceding
arguments, just as if they had no default values. So the default values are no help to us;
it's almost like they weren't there at all! In fact, this refactoring would have been much easier
if all five of the parameters _had_ been mandatory; the ability to omit trailing parameters actually
makes manual refactoring _harder_.

Conclusion: Default function arguments do not directly cause boolean parameter tarpits, but they
make their pernicious effects worse. Boolean tarpits that _don't_ use default arguments
are easier to deal with than boolean tarpits that _do_ use default arguments.


## Fiddly redeclaration rules

A default function argument appears syntactically in a function _declaration_, but the default argument itself
behaves like a _definition_, in that there must be only one of it per translation unit.

    int f(int x = 1);
    int f(int x = 1);  // ERROR: redefinition of default argument

Even in the function definition, you aren't allowed to repeat the default argument's value.
This means that a piece of the function's interface is invisible at the site of the function's
implementation. I frequently see workarounds such as this:

    int f(int x/*=1*/) {
        return x;
    }

But, like any code comment, this `/*=1*/` could easily get out of sync with reality.

    int f(int x = 2);

    int f(int x/*=1*/) {   // OOPS!
        return x;
    }


## Default function arguments can't depend on other arguments

    void draw_rectangle(int width, int height = width);  // ERROR

If you want this effect, you _must_ use one overload per signature.

    void draw_rectangle(int width, int height);
    inline void draw_rectangle(int width) { draw_rectangle(width, width); }


## Surprising function-pointer type

Consider the following snippets ([Godbolt](https://godbolt.org/z/-CTa9D)):

    int nexta(int x, int y) { return x+y; }
    int nexta(int x) { return nexta(x, 1); }

    int nextb(int x, int y = 1) { return x+y; }

    auto nextc = [](int x, int y = 1) { return x+y; };

    assert(nexta(10) == 11);
    assert(nextb(10) == 11);
    assert(nextc(10) == 11);

    int (*pfa)(int) = nexta;  // OK
    int (*pfb)(int) = nextb;  // ERROR!
    int (*pfc)(int) = nextc;  // ERROR!

`nextb` is callable with only one argument; but that doesn't make it actually _be_ a function
of only one argument. This pitfall combines with
["Hidden `reinterpret_cast`s"](/blog/2020/01/22/expression-list-in-functional-cast/) (2020-01-22)
to produce a [startling result](https://godbolt.org/z/9o2Vkp):

    using FP = int(*)(int);
    printf("%d\n", FP(nexta)(10));   // OK, works fine
    printf("%d\n", FP(nextb)(10));   // garbage at runtime
    printf("%d\n", FP(nextc)(10));   // nice compile-time error

In fairness, I must point out that `nexta` and `nextc` reverse their roles
when we look at `std::function`. (But `nextb` is still the worst of all worlds.)

    std::function<int(int)> fa = nexta;  // ERROR, ambiguous
    std::function<int(int)> fb = nextb;  // ERROR!
    std::function<int(int)> fc = nextc;  // OK


## Interaction with inherited functions

I've never seen this pitfall in real code... maybe because I never use
default arguments _and_ always use the Non-Virtual Interface idiom.
But it's so well-known that the C++ Core Guidelines devote
[rule C.140](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rh-virtual-default-arg) to it.
[Godbolt:](https://godbolt.org/z/sb8mYE)

    struct Cat {
        virtual void speak(const char *s = "meow")
            { printf("%s.\n", s); }
    };
    struct Tiger : public Cat {
        void speak(const char *s = "roar") override
            { printf("%s!!\n", s); }
    };

    Cat c;       c.speak();  // "meow."
    Tiger t;     t.speak();  // "roar!!"
    Cat *p = &t; p->speak(); // "meow!!"

Notice that if we call `Tiger::speak` through the pointer-to-base `p`,
we get a weird hybrid of `Cat` and `Tiger` behavior. This is absolutely not what we want
under any circumstances. (The NVI idiom would protect us from this pitfall, because in NVI
we would never be calling a virtual method directly in the first place.)

Worse, C++ forces us to _repeat_ the default argument's value at each level of the hierarchy:

    struct Cat {
        virtual void speak(const char *s = "meow")
            { printf("%s.\n", s); }
    };
    struct Tiger : public Cat {
        // Can I piggyback on Cat's default argument here? Nope!
        void speak(const char *s) override
            { printf("%s!!\n", s); }
    };

    Tiger t;  t.speak();  // ERROR! Tiger::speak needs an argument

If we use default function arguments in a polymorphic hierarchy, we must repeat
ourselves _more_ often, not less.

For the record, the NVI solution to this problem [would be](https://godbolt.org/z/9QHZtR):

    struct Cat {
        void speak() { do_speak(do_default()); }
        void speak(const char *p) { do_speak(p); }
    private:
        virtual const char *do_default()
            { return "meow"; }
        virtual void do_speak(const char *s)
            { printf("%s.\n", s); }
    };
    class Tiger : public Cat {
        const char *do_default() override
            { return "roar"; }
        void do_speak(const char *s) override
            { printf("%s!!\n", s); }
    };

    Cat c;       c.speak();  // "meow."
    Tiger t;     t.speak();  // "roar!!"
    Cat *p = &t; p->speak(); // "roar!!"

And if you're wondering whether `void speak(const char *p = do_default())` would work...
[no, it doesn't.](https://godbolt.org/z/lyRWcI)


## Side effects and name lookup on default arguments can be confusing

Default arguments are _defined_ in one place but _evaluated_ in another.
This can lead to confusion about aspects of C++ that are somewhere in the middle, such as
name lookup, access control, and template instantiation.

    int v = 0;
    void f(int x = ++v);

`f()` causes an increment to `v`, but `f(0)` does not.

    template<class T>
    void ft(T x = T());

`ft<X>()` requires that `X` be default-constructible, but `ft<X>(x)` does not.

    template<class T>
    void ft2(int x = T::member());

`ft2<X>()` requires that `T::member()` exist and be accessible in the context of `ft2`
(not in the caller's context); but `ft<X>(0)` does not.

And [in this complex example](https://godbolt.org/z/S9J8XS), we see that while
default arguments may be _evaluated_ at the call-site, their _name lookup_ happens
via lookup (or in the case of templates, two-phase lookup) in the callee's context.

The reason I find all of these examples confusing (to varying degrees) is that default function
arguments exist in a gray area — they kinda belong to the scope of the callee function, but they kinda
seem to belong to the scope of its caller, too (since that's where they're evaluated).
Contrariwise, when I write

    template<class T> void ft2(int x);
    template<class T> void ft2() { return ft2<T>(T::member()); }

I find it easy to remember what rules to follow for name lookup, access control,
"when side effects happen," and so on, because they're the usual lexical-scope-based rules
that we all use in C++ every day.


## Conflation of `explicit` and non-`explicit` constructor signatures

Another of my pet guidelines is:

> [All constructors should be `explicit`.](/blog/2023/04/08/most-ctors-should-be-explicit/)
> (Except for move and copy, because C++ makes implicit copies all over the place.)

So for example I would write

    template<class T, class Comparator, class Container>
    class priority_queue {
    public:
        explicit priority_queue() : priority_queue(Compare(), Container()) {}
        explicit priority_queue(const Compare& p) : priority_queue(p, Container()) {}
        explicit priority_queue(const Compare&, const Container&);
        explicit priority_queue(const Compare&, Container&&);
    };

The actual STL follows the guideline "All _single-argument_ constructors should be `explicit`,
and all zero-argument and multi-argument constructors should be non-`explicit`." (An implicit multi-argument
constructor can be used to do initialization from a braced sequence, like how `std::pair<int,char>` can be
constructed from `return {1, 'x'}`. Implicit conversion like this is good for sequence-like vocabulary types
like `tuple`, but I claim it's surprising when `{std::less<>(), myVec}` is quietly
interpreted as a `priority_queue` object! Anyway...) The STL wants to write

    template<class T, class Comparator, class Container>
    class priority_queue {
    public:
        priority_queue() : priority_queue(Compare(), Container()) {}
        explicit priority_queue(const Compare& p) : priority_queue(p, Container()) {}
        priority_queue(const Compare&, Container&&);
        priority_queue(const Compare&, const Container&);
    };

Unfortunately, C++11 specified this overload set in terms of default function arguments, like this:

    template<class T, class Comparator, class Container>
    class priority_queue {
    public:
        explicit priority_queue(const Compare& = Compare(), Container&& = Container());
        priority_queue(const Compare&, const Container&);
    };

Leaving off the `explicit` keyword would have created an implicit conversion from `Compare` to `priority_queue`,
which would have been bad; but _adding_ `explicit` accidentally made the default constructor and the two-argument
constructor `explicit`, which is also bad!

The Committee's solution was to eliminate the default function arguments and use a full overload set instead,
with one overload per function signature. A _lot_ of paper was expended on this migration.
Alisdair Meredith's [P1374](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1374r0.html) (November 2018)
contains a detailed list of work in this area. For the specific example of `std::priority_queue`, see
Tim Song's [P0935](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0935r0) (February 2018).

Again, you _can_ work around this issue by carefully crafting your overload sets, still using
default function arguments where it's safe and avoiding them where needed. But recognize that by
doing that, you're creating a needless puzzle for your code's maintainer!


## Conflation of `noexcept` and non-`noexcept` function signatures

Consider [`std::shared_ptr<T>::reset()`](https://en.cppreference.com/w/cpp/memory/shared_ptr/reset).
Before you look at that cppreference page, ask yourself: Could we declare it like this?

    void reset(T *ptr = nullptr);

It turns out to be preferable to declare it as an overload set — one overload per signature — because
the first of those signatures can be `noexcept`!

    void reset() noexcept;
    void reset(T *ptr);

The one-argument version of `reset` needs to allocate a control block, which can fail and throw `bad_alloc`.
The zero-argument version never allocates and thus never throws. So, even though `sp.reset()` is semantically
equivalent to `sp.reset((T*)nullptr)`, there is still a salient difference between them at the language level
which causes the STL to prefer separate overloads instead of default function arguments.


## The answer to "can I do this?" is always no

    auto lam = [](auto x = 1) { return x; };
    lam();

Does `lam()` call `lam(1)`? Nope, it fails type deduction.

    struct S {
        int default_value();
        void f(int x = default_value());
    };

Nope! (See [above](#and-if-you-re-wondering-whether).)

    int f(int x = 1, int y);
    int debugf(const char *fmt, ..., int y = 1);

Of course not!

    MyType operator+(MyType x, MyType y = {});

No way!


## Buggy compilers

I was surprised to learn there are still compiler bugs in such an old (1980s-era) feature.
But in 2022 I learned about a longstanding bug in both GCC ([#96645](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96645))
and Clang ([#36032](https://github.com/llvm/llvm-project/issues/36032)):

    struct A {
        struct B {
            int i = 42;
        };
        static void f(B b = B());
    };

Both GCC and Clang reject this valid code, seemingly because they resist generating an
implicitly declared default constructor before the end of an enclosing class — even when
the constructor is `B`’s but the enclosing class is `A`! Work around this default-argument
bug by eliminating default arguments: every vendor is happy with the following code,
and it's clearer to boot.

    struct A {
        struct B {
            int i = 42;
        };
        static void f() { f(B()); }
        static void f(B b);
    };


## What about `std::source_location`?

C++20 gives students a new reason to ask about default function arguments: `std::source_location`.
See ["How to replace `__FILE__` with `source_location` in a logging macro"](/blog/2020/02/12/source-location/) (2020-02-12).
`std::source_location::current()` is designed to piggyback on default arguments, via a special
compiler-magic mechanism that is not available to user-defined types:

    void f(int x, std::source_location loc = std::source_location::current()) {
        printf("%s %"PRIdLEAST32"\n", loc.file_name(), loc.line());
    }

However, because of the poor design decision to rely on a default function argument:

- The `source_location` argument must go in the trailing position...

- which means that C++ can never add anything else that works this way: `source_location` has laid permanent claim to
    the real estate at the end of a function argument list.

- That defaulted function parameter is "hijackable" by the caller, just like `enable_if` template parameters
    before C++20's invention of the `requires` keyword.

- [Overloaded operators can't use it](https://lists.isocpp.org/std-proposals/2020/03/1154.php),
    because they can't take default function arguments.

- Varargs functions like `printf` can't use it.

- Variadic templates like `make_unique` can't use it ([as far as I can tell](https://godbolt.org/z/EKZJKj) — am I wrong?).

In short, it's of very limited usefulness, it abuses an already pitfall-heavy mechanism, and generally I wouldn't
use it.

The `std::source_location` type _itself_ is pretty neat. There was lots of discussion about how to make it
perform efficiently. (It's not just a tuple of strings and ints!) But the chosen idiom for
getting at `source_location::current()` is silly and hard to use — much harder than just continuing to use
`__FILE__` and `__LINE__`. And the primary thing that _makes_ it silly and hard to use is that
it relies on default function arguments!

I see the experience of `std::source_location` as further evidence that default function arguments are the devil.


## What about default _template_ arguments?

I have no complaints about default template arguments. The STL uses them to very good effect, for things
like allocator and comparator parameters. Templates are a vastly different animal from functions.
In particular, C++ doesn't let us overload templates, so default template arguments are our only tool
if we want to make `set<T>` and `set<T, std::greater<T>>` both valid.

I do [intensely disapprove of CTAD](/blog/tags/#class-template-argument-deduction),
so ever since C++17 I've had to say "be careful when defaulting template parameters that you don't accidentally
introduce a CTAD issue." See ["Contra P0339"](/blog/2019/07/02/contra-p0339/#the-interaction-with-ctad-and-common-typos)
(2019-07-02) for one example. But that's a CTAD problem, not a default-template-argument problem —
just [avoid CTAD](/blog/2018/12/09/wctad/) and you'll be fine.


## Conclusion

- Default function arguments are the devil.

- Any time you are considering using a default function argument, it's because you're crafting an overload set.

- Figure out the exact signatures you mean to provide, and write each of these signatures as a separate function declaration.

- Don't cram multiple signatures into one declaration.

- Don't create puzzles for your code's maintainer.
