---
layout: post
title: "What is the `std::swap` two-step?"
date: 2020-07-11 00:01:00 +0000
tags:
  argument-dependent-lookup
  c++-learner-track
  library-design
  wg21-folkloristics
---

This blog post has now been more than a year in the making; it's the one
for which I was laying the groundwork when I wrote ["What is ADL?"](/blog/2019/04/26/what-is-adl/) (2019-04-26).
Since I used the term again the other day, let's finally define it:
What is the `std::swap` two-step?

The name "`std::swap` two-step" comes (as far as I know) from Eric Niebler's blog post
["Customization Point Design in C++11 and Beyond"](http://ericniebler.com/2014/10/21/customization-point-design-in-c11-and-beyond/) (October 2014).
It has also been called the "`std` two-step" and the "ADL two-step."


## Background

Recall that when we write an unqualified function call such as `foo(x, y)`,
the compiler will look up the name `foo` not only in our current namespace
but also in the namespaces associated with the types of `x` and `y`. This (mis)feature
of C++ is known as argument-dependent lookup, or ADL. [Godbolt:](https://godbolt.org/z/9afceY)

    namespace N1 {
        struct A {};
        void foo(A&);
    }
    namespace N2 {
        using N1::A;
        A a;
        void foo(A&);
    }
    namespace N3 {
        using N1::A;
        void foo(const N1::A&);

        void test() {
            foo(N2::a);  // calls which foo?
        }
    }

Here, the unqualified call `foo(N2::a)` calls `N1::foo`, because the type of `N2::a`
(after "looking through" all using-declarations and type aliases) is `N1::A`.
Overload resolution considers both `N1::foo` (found via ADL) and `N3::foo`
(found via regular lookup) and selects `N1::foo` as the better-matching candidate.
`N2::foo` is a red herring.

ADL is good for creating customization points in the same way that a sledgehammer is good for driving
screws (see ["Customization point design for library functions"](/blog/2018/03/19/customization-points-for-functions/)
(2018-03-19) and
["C++ doesn't know how to do customization points that aren't operators"](/blog/2018/08/23/customization-point-or-named-function-pick-one/)
(2018-08-23)). When a type author wants to say "Here's how you _add_ two values of my type," they do so by
providing an `operator+` findable by ADL. When a type author wants to say "Here's how you _print_ an object
of my type," they provide an `operator<<` findable by ADL. And when they want to say "Here's how you _swap_
two objects of my type," they provide a `swap(X&, X&)` findable by ADL.

The canonical way to do this, in my opinion, is to use the _hidden friend idiom_ (which is another thing
I mean to blog about someday):

    namespace N {
        class MyType {
            MyType& operator+=(const MyType&);
            void print(std::ostream&);
            void swap(MyType&) noexcept;

            friend MyType operator+(MyType a, const MyType& b) {
                a += b;
                return a;
            }
            friend std::ostream& operator<<(std::ostream& os, const MyType& b) {
                b.print(os);
                return os;
            }
            friend void swap(MyType& a, MyType& b) noexcept {
                a.swap(b);
            }
        };
    } // namespace N

It would work just as well in this case to make those hidden friends into members of
`MyType`'s namespace `N`, as follows. I never recommend this:

    namespace N {
        class MyType {
            MyType& operator+=(const MyType&);
            void print(std::ostream&);
            void swap(MyType&) noexcept;
        };
        MyType operator+(MyType, const MyType&);
        std::ostream& operator<<(std::ostream&, const MyType&);
        void swap(MyType&, MyType&) noexcept;
    }

Anyway, the upshot of this is that when some client code — some _generic_ code, perhaps —
makes an unqualified function call to one of these names, they'll trigger ADL and ADL will
find these candidates.

    void test(N::MyType& t) {
        t + t;           // considers N::operator+
        std::cout << t;  // considers N::operator<<
        swap(t, t);      // considers N::swap
    }


## What about types that haven't customized their behavior?

What happens if we have a type like `struct S` that hasn't customized its behavior
for addition, printing, or swapping?

    struct S {};
    void test(S& s) {
        s + s;           // ERROR!
        std::cout << s;  // ERROR!
        swap(s, s);      // ERROR!
    }

In the first two cases, it makes sense: You can't _add_ instances of a class type
that doesn't explicitly overload `+`, and you can't _print_ instances of a class type
that doesn't explicitly overload `<<`. But (I claim) it doesn't really make sense
to say that you can't _swap_ instances.

There are some other operations that C++ lets you do on "vanilla" class types like `S`:
default-construction, copy-construction, copy-assignment, and destruction. C++ handles
those operations by generating an _implicitly defaulted definition_ for each of those
operations. C++ could have said that it'd also generate an implicitly defaulted definition
for `swap`; but history didn't go that way.

Instead, the standard library provides a sort of "hand-written default implementation"
for `swap`, and leaves it up to the client code to glue the pieces together correctly.
In generic code, where you don't know whether your type `T` has an ADL candidate for
`swap` or not, you want basically this high-level behavior:

    template<class T>
    void test(T& t) {
        if (t has ADL swap) {
            swap(t, t);  // using the ADL candidate
        } else {
            std::swap(t, t);  // using the library's hand-written default
        }
    }

It turns out that we can achieve this behavior very easily, because the STL's `std::swap`
is deliberately written as the least-constrained kind of template possible, meaning that
it ranks lowest in the overload resolution scheme. _Any_ viable candidate found via ADL
ought to rank higher than `std::swap`. So in real life, we just need to make sure that
name lookup will always consider `std::swap` a candidate in addition to the ADL candidate(s)
(if any). We write this:

    template<class T>
    void test(T& t) {
        using std::swap;  // make it a candidate
        swap(t, t);
    }

And here we finally have the "two-step."

* Step 1: `using std::swap;`

* Step 2: Unqualified call to `swap(t, t)`.


## When should I use the two-step?

The `std::swap` two-step solves a problem in _static dispatch_ that's very similar to a problem
you might already know from _dynamic dispatch_.

    class Base {
        void frotz() { puts("Light"); }
        virtual void xyzzy() { puts("Debris"); }
        virtual void plugh() = 0;
    };

    class Derived : public Base { ... };

    void test(Base *b) {
        b->frotz();
        b->xyzzy();
        b->plugh();
    }

In C++, where static dispatch is the status quo, an ordinary non-virtual method like `frotz` is basically
the type author telling the program, "I know exactly how to `frotz` myself. This is how you do it.
Nobody else will ever know better than me how to `frotz`."

A virtual method like `xyzzy`, in contrast,
indicates, "I know how to `xyzzy` myself, but my child class might know a better way. If my child has
an opinion, you should trust my child over me."

A _pure_ virtual method like `plugh` indicates, "Not only should you trust my child over me, but I'm
not giving you a choice! I have _no idea_ how to `plugh` myself. My child _must_ come up with a solution.
I offer no guidance here."

Translating this into the world of static dispatch and ADL, we have something like this:

    namespace base {
        template<class T> void frotz(T t) { puts("Light"); }
        template<class T> void xyzzy(T t) { puts("Debris"); }
    }

    template<class T>
    void test(T t) {
        base::frotz(t);
        using base::xyzzy; xyzzy(t);
        plugh(t);
    }

A qualified call like `base::frotz(t)` indicates, "I'm sure I know how to `frotz` whatever this thing may be.
No type `T` will ever know better than me how to `frotz`."

An unqualified call using the two-step, like `xyzzy(t)`, indicates, "I know how to `xyzzy`
whatever this thing may be, but type `T` might know a better way. If `T` has an opinion,
you should trust `T` over me."

An unqualified call _not_ using the two-step, like `plugh(t)`, indicates, "Not only should you trust `T`
over me, but I myself have no idea how to `plugh` anything. Type `T` _must_ come up with a solution;
I offer no guidance here."

Notice that in the `virtual` world, `class Base` itself has control over which calls dispatch to the child
and which don't. In the static-dispatch world, it's the _client code_, `test`, that decides
what kind of call to use in each situation.


### Should I use the two-step in non-generic code?

<b>No.</b> Sometimes I see students want to do this:

    class Book {
        std::string title_;
        int pagecount_;
    public:
        void swap(Book& rhs) noexcept {
            using std::swap;
            swap(title_, rhs.title_);
            swap(pagecount_, rhs.pagecount_);
        }
    };

All else being equal, the two-step here is just obfuscation. We know that `title_` is a `std::string`;
we know what candidate we want to use to swap strings. We also know what candidate we want to use to swap ints.
I would prefer to see this written simply as

        void swap(Book& rhs) noexcept {
            std::swap(title_, rhs.title_);
            std::swap(pagecount_, rhs.pagecount_);
        }

or even

        void swap(Book& rhs) noexcept {
            title_.swap(rhs.title_);
            std::swap(pagecount_, rhs.pagecount_);
        }

(Note that `std::swap` is overloaded for strings, so `std::swap(s1, s2)` and `s1.swap(s2)` end up doing the
exact same thing. The latter is no more efficient than the former; it's just shorter to type.)


### Should I use the two-step to provide a fallback in generic code?

<b>Yes.</b> The "`std::swap` two-step" gets its name from exactly this use-case. The STL provides a
_fallback implementation_ of `swap` that we want to use specifically for types `T` that haven't provided
any better candidate.

You should use the two-step in any similar situation. In my experience, these situations
are very few and far between. _Theoretically,_ you should use the two-step on customization points like
`begin` and `end`:

    template<class R>
    void mysort(R& range) {
        using std::begin;
        using std::end;
        std::sort( begin(range), end(range) );
    }

This means that `mysort<N::MyType>` will consider using `N::begin` and `N::end` if they exist and are better
matches than the generic `std::begin` and `std::end`. However, _in my experience_, this is never actually
the case. Lots of type authors provide an ADL `swap`. No type author has ever provided an ADL `begin`.

> However, should you use `std::begin(x)` instead of `x.begin()`?
> In generic code, yes; because `std::begin(x)` works for array types like `int[10]`
> whereas `x.begin()` works only for class types.

UPDATE, 2020-07-12: Reddit points out that `std::filesystem::directory_iterator` is a type with an ADL `begin` and no
member `begin`. I should have remembered this because I wrote the rejected proposal
[P0757 "`regex_iterator` should be iterable"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0757r0.html)
(September 2017)! Michael Hava reports that Microsoft's C++/CX also provides an
ADL [`begin`](https://docs.microsoft.com/en-us/cpp/cppcx/begin-function)
taking an [`IVector<T>^`](https://docs.microsoft.com/en-us/uwp/api/windows.foundation.collections.ivector-1)
(both in the `Windows::Foundation::Collections` namespace), thus enabling you to loop `for (auto&& elt : myVec)`
without dereferencing `myVec` first. These are good examples of real types providing ADL `begin`
in lieu of member `begin`. "Good _example_" does not mean "good _role model_," though!


### Should I use the two-step in generic code lacking a fallback?

<b>No.</b> See the end of ["How to erase from an STL container"](/blog/2020/07/08/erase-if) (2020-07-08),
where I say that it would be wrong wrong wrong to do this:

    template<class C>
    void remove_all_odd_elements(C& c) {
        using std::erase_if;
        erase_if(c, [](int x) { return x % 2; });
    }

There are two possible readings here: Either the author of this generic code intends `erase_if` to be like
`frotz` — "No type `T` will ever know better than me how to `frotz` things" — or else the author intends it
to be like `xyzzy` — "I know how to `xyzzy` things, but if `T` has an opinion, you should trust `T` over me."

In the former case, bringing ADL into the mix is simply incorrect. The author should have written:

    template<class C>
    void remove_all_odd_elements(C& c) {
        std::erase_if(c, [](int x) { return x % 2; });
    }

In the latter case, the author is permitting `C` to have a say in the meaning of `erase_if`; but also
(by using the two-step) saying that it's okay to fall back to `std::erase_if` if `C` doesn't provide
any viable `erase_if` via ADL. The problem with this is that the `erase_if` in namespace `std` works
_only for container types that themselves are already in namespace `std`!_ There is no situation where
`std::erase_if` would ever actually work as a "fallback." Therefore, the author should simply have written:

    template<class C>
    void remove_all_odd_elements(C& c) {

        // use ADL so we'll consider
        // MyLib::erase_if(MyLib::FlatMap&, Predicate)
        // as a candidate for this call

        erase_if(c, [](int x) { return x % 2; });
    }

This is analogous to how you wouldn't write `using std::operator+;` in front of a call to `c + c`.
There's simply no situation in which the ADL call would have failed and yet some overload of
`std::operator+` would have worked.

However, as I said in that previous blog post, I _really_ don't think this is a good idea
in the specific case of `erase_if`. Call `std::erase_if` with qualification, the same way
you'd call `std::rethrow_exception` or `std::from_chars`.


## Can't I just use C++20 `std::ranges::swap`?

<b>Yes,</b> my understanding is that in C++20 you can just write

    #include <concepts>

    template<class T>
    void test(T& t) {
        std::ranges::swap(t, t);
    }

`std::ranges::swap` is not a function
but a [CPO](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#cpo),
i.e., a global variable with a templated `operator()` that has been defined to do the two-step internally
so that you don't have to.

But for those who need portability to C++17-and-earlier; or for those who might encounter those
rare non-`swap` customization points; or for those who just like to understand what
`std::ranges::swap` is doing (and why!); I hope this blog post helped.
