---
layout: post
title: '`std::string_view` is a parameter-only type'
date: 2018-03-27 00:01:00 +0000
tags:
  c++-style
  lifetime-extension
  paradigm-shift
  parameter-only-types
  pitfalls
---

Today I'd like to talk about [`std::string_view`](http://en.cppreference.com/w/cpp/string/basic_string_view).
`string_view` arrived in C++17, amid quite a bit of confusion about what exactly it's *for* and how to
use it safely.

The basic idea of `string_view` is that it lets you hit a sweet middle ground between C++03-style concreteness:

    class Widget {
        std::string name_;
    public:
        void setName(const char *new_name);
        void setName(const std::string& new_name);
    };

and full-on generic programming, which in a pre-Concepts, C++17 world essentially means choosing between
inappropriately underconstrained templates:

    class Widget {
        std::string name_;
    public:
        template<class T>
        void setName(T&& new_name);
          // defined elsewhere in this header
    };

and properly constrained but comically verbose templates:

    class Widget {
        std::string name_;
    public:
        template<class T, class = decltype(std::declval<std::string&>() = std::declval<T&&>())>>
        void setName(T&& new_name);
          // defined elsewhere in this header
    };

With `std::string_view`, we can write simply this:

    class Widget {
        std::string name_;
    public:
        void setName(std::string_view new_name);
    };

I believe that `string_view` succeeds admirably in the goal of "drop-in replacement for `const string&`
parameters." The problem I have observed, at CppCon 2017 and at Albuquerque 2017, is that some people
(you know who you are) persist in trying to use `string_view` as a drop-in replacement for
`const string&` in _all_ circumstances! And then they get bitten:

    const std::string& s1 = "hello world";  // OK, lifetime-extended
    const std::string& s2 = std::string("hello world");  // OK, lifetime-extended
    std::string_view sv1 = "hello world";  // OK, points to static array
    std::string_view sv2 = std::string("hello world");  // BUG! Dangling pointer!

Or again:

    auto identity(std::string_view sv) { return sv; }

    int main() {
        std::string s = "hello";
        auto sv1 = identity(s);  // OK
        auto sv2 = identity(s + " world");  // BUG! Dangling pointer!
    }

This example is more subtle, because the sin (using `string_view` as the return type of
the `identity` function, and then later using `string_view` as the type of local variables
`sv1` and `sv2`) is hidden behind the `auto` keyword. `auto` may *hide* your sins, but
it does not *expiate* them! Using `string_view` as anything other than a parameter type
is always wrong.

This suggests to me that we have a relatively new entrant into the field of
"well-understood kinds of types" in C++. The two relatively old kinds of types are
*object* types and *value* types. The new kid on the block is the *parameter-only* type.

- *Object* types are generally immobile (deleted copy constructor, for example) and lack
  an assignment operator; they are identified by memory address; they rely on mutation;
  they may be classically polymorphic.

- *Value* types are identified by "value." They have strong ownership semantics.
  You can work with them in a functional, mutation-free style; they lend themselves
  well to being moved around in memory, stored in `tuple`s, returned by value from
  functions, and so on.

- *Parameter-only* types are essentially "borrowed" references to existing objects.
  They lack ownership; they are short-lived; they generally can do without an
  assignment operator. They generally appear only in function parameter lists;
  because they lack ownership semantics, they generally cannot be stored in
  data structures or returned safely from functions.

> UPDATE, 2023-07-19: [The original of this post](https://web.archive.org/web/20180404223442/https://quuxplusone.github.io/blog/2018/03/27/string-view-is-a-borrow-type/)
> used the term "borrow type," borrowed from Rust; but soon I decided that "parameter-only type" was a much better name,
> both because it's more explanatory and because it avoids confusion with the Rust idea,
> which is after all fairly different. I added a note to that effect in May 2019, but
> it took me until July 2023 to actually re-edit this post.
>
> C++20 Ranges introduced the term ["borrowed range"](https://en.cppreference.com/w/cpp/ranges/borrowed_range)
> for a view which can safely be "deboned" into iterators. Views are generally parameter-only
> types, even when they don't satisfy the C++20 definition of a "borrowed range."

`string_view` is the first "mainstream" parameter-only type. But C++ has other
types that arguably match the criteria above:

- `std::reference_wrapper<T>`. We see this type used as a function parameter to the constructor
  of `std::thread` and to functions like `std::make_pair` and `std::invoke`. We do not see
  the STL using it as anything other than a parameter type.

- `std::tuple<Ts&...>`, the result type of `std::forward_as_tuple(Ts...)`. This is an interesting
  one to me personally, because if I were designing the STL, I would have made this a completely
  separate class template, rather than recycling `std::tuple`. Notice that `std::tuple<Ts...>`
  is a *value* type according to our classification above; but `std::tuple<Ts&...>` is a
  parameter-only *view* type. To me, this change in semantics signals a mistake on par with `vector<bool>`.

- `T&&`, either in the sense of "rvalue reference" (when `T` is some known object type) or
  in the sense of "forwarding reference" (when `T` is deduced). We see this type used as a
  function parameter all over the place. We rarely see it in any other context, unless we
  are doing metaprogramming specifically on reference-type value categories. (I mean,
  the function `std::move` itself does return an rvalue reference. But in normal everyday code,
  returning an rvalue reference is a sure sign that you're doing something wrong.)

- And now, `std::string_view`.

Notice that because all of the above types were designed by [Not Me](https://www.retroist.com/2012/09/13/family-circus-not-me-and-ida-know/),
they all break some of the general rules for parameter-only types:

- `reference_wrapper<T>` is assignable: `rw1 = rw2`. [Assignment has shallow semantics](https://wandbox.org/permlink/RxH2TwIDLFC5fWsg),
  even though comparison `rw1 == rw2` has deep semantics.

- `tuple<Ts&...>` is assignable: `t1 = t2`. Assignment has deep semantics, as does
  comparison.

- `T&&` is actually pretty great. It automatically has reference semantics for everything,
  because, well, it's a reference.

- `string_view` is assignable: `sv1 = sv2`. Assignment has shallow semantics (of course —
  the viewed strings are immutable), even though comparison `sv1 == sv2` has deep semantics.

If I were designing `reference_wrapper` and `string_view`, I would have given them no assignment
operator at all; this would prevent any confusion about the meaning of assignment, and additionally
removed a perennial complaint: that these types provide both copy/assignment and comparison, but
in an inconsistent manner that makes them not [Regular](http://stepanovpapers.com/DeSt98.pdf) types.

I originally called these types *borrow types* (despite the confusion that would generate),
because borrowing is the fundamental property that allows us to reason about
their semantics. You should use a parameter-only type if and only if it is *short-lived* enough
that it can safely (that is, unsafely) *borrow* ownership of another object and then
go away again before the other object notices. In practice, this means it's got to be
either a function parameter or a ranged-for-loop control variable;
and you can't do anything with the value of the parameter that requires
it to outlive the function or loop.

    struct StringSet {
        vector<string> elements_;

        template<class F>
        void for_each_element(const F& f) const {
            for (string_view elt : elements_) {
                f(elt);
            }
        }
    };

In the above code sample, callback `f` is "borrowed" only for the lifetime of `for_each_element`;
therefore it is correct to take it as the parameter-only type `const F&` instead of as the
(potentially more expensive) value type `F`.

Likewise, `elt` is "borrowed" only for the lifetime of the for-loop; therefore it is
correct to take it as `string_view` instead of as the (potentially
more expensive) value type `string`. Notice that we could equally well take it as
the parameter-only type `auto&&` ([and perhaps we should](/blog/2018/12/15/autorefref-always-works/)),
but I wanted to demonstrate a valid use of `string_view` that proves it might be useful
outside a _function_ parameter list.

I would hope that static analysis tools such as the C++ Core Guidelines would explicitly
call out this pattern of "borrowing," but it appears that they have not caught up
just yet. The Core Guidelines discussion around `string_view` in particular seems to
have gotten bogged down in [trying to *avoid* diagnosing sinful code](https://github.com/isocpp/CppCoreGuidelines/issues/1038),
which I'd say is the exact opposite of the Core Guidelines' original mandate.


## Returning parameter-only types

One must be very careful when returning a parameter-only type from a getter method:

    class Widget {
        string name_;
    public:
        string_view getName() const { return name_; }
    };

    Widget w;
    std::cout << w.getName() << std::endl;

It's safe to pass the parameter-only object returned from `w.getName()` as a parameter to
`operator<<`; but it would be absolutely incorrect to capture it
into a local variable which is *not* a function parameter:

    auto sv1 = w.getName();  // SIN! Parameter-only type used as non-parameter!
    w.setName("hello world");  // "Borrowed" object's lifetime ends
    std::cout << sv1 << std::endl;  // BUG! Dangling pointer!

Therefore I think we should frown on returning parameter-only object types such as `string_view`
from functions (without a very good reason). They're just too easy to misuse. Note that
native reference types like `const string&` don't have this particular problem, because
for them, `auto` will make a copy. (But see
["On `function_ref` and `string_view`"](/blog/2019/05/10/function-ref-vs-string-view/) (2019-05-10)
for an even subtler take on "making copies.")


## Simple rules for parameter-only types

The rule of thumb is simple and statically checkable:

- Parameter-only types must appear *only* as function parameters and for-loop control variables.
  
We can make an exception for return types:

- A function may have a parameter-only type as its return type, but if so, the function must
  be explicitly annotated as returning a potentially dangling reference. That is, the
  programmer must explicitly acknowledge responsibility for the annotated function's
  correctness.

- Regardless, if `f` is such a function, the result of `f` must not be stored
  into any named variable except a function parameter or for-loop control variable.
  For example, `auto x = f()` must still be diagnosed as a violation.

Follow these rules and you shouldn't get into any trouble with `std::string_view`.
It's perfectly usable and will even simplify your code,
*if you follow the rules for parameter-only types in C++.*
