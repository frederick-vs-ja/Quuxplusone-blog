---
layout: post
title: 'What does the Standard mean by "`_traits`"?'
date: 2018-07-14 00:01:00 +0000
tags:
  allocators
  customization-points
  library-design
  metaprogramming
  templates
  wg21
---

This is the first sequel to my post several months ago on
[customization point design for library functions](/blog/2018/03/19/customization-points-for-functions).
That post was all about _function_ customization points. This post is all about _type-based_ customization
points — a.k.a. traits classes — or at least what the standard library means when *it* says "traits class."

If you're just looking for a tutorial introduction to traits classes, without all the nitpicky wiffling
I'm about to do in this blog post, I highly recommend that you stop reading and go watch
my two-hour CppCon 2016 session on ["Template Normal Programming"](https://www.youtube.com/watch?v=vwrXHznaYLA)
([Part 1](https://www.youtube.com/watch?v=vwrXHznaYLA), [Part 2](https://www.youtube.com/watch?v=VIz6xBvwYd8));
and/or, Vinnie Falco's CppCon 2017 session ["Make Classes Great Again"](https://www.youtube.com/watch?v=WsUnnYEKPnI).
This post (as with [much of my content](https://amzn.to/2CTujmN)) is intended mostly for people who
already think they know this stuff backward and forward.

----

Readers of my previous post on [customization point design for library functions](/blog/2018/03/19/customization-points-for-functions)
(including yourself... right?) will recall that any function-based customization point has two pieces:

* A, the piece the user is required to specialize; and

* B, the piece the user is required to _invoke_ (and therefore must _not_ specialize).

Let's take "hashing" as our first example.

    template<class T>
    struct hash;  // primary template provided by the library writer

    template<class T, class Hash = hash<T>>
    struct unordered_set {
        // uses Hash::operator()(const T&)
    };

    // provided by the programmer
    class X;

Here, the user-programmer's ultimate goal is to put items of type `X` into an `unordered_set`.
The library writer has determined that `unordered_set<T>` needs some way to hash items of type `T`.
So the library writer introduces a "policy parameter" to their template. They turn `unordered_set<T>`
into `unordered_set<T, Hash>`. Here `Hash` is kind of like piece "B"... but I don't think it's quite analogous,
because of what we're going to do next.

Generally, user-programmers don't like to write code; so the library writer ought to provide some sort
of *sensible default value* for `Hash`. The library writer defaults `Hash` to `hash<T>`.

Now the user-programmer has two plausible alternatives for how to use `unordered_set`.
The first alternative is to "make `X` hashable" by providing a new specialization of the default `hash<T>`:

    // specialization provided by the programmer
    template<>
    struct std::hash<X> { size_t operator()(const X&); };

    unordered_set<X> s;  // Success!

The second alternative is to "define a hash function over `X`s" and use that new hash function
when creating the set:

    // new type provided by the programmer
    struct XHasher { size_t operator()(const X&); };

    unordered_set<X, XHasher> s;  // Success!

In the C++ standard library, both approaches are valid. The first approach is more common, both because
it is more convenient and because it is more *generic*: it automatically affects every place in the program
where an `unordered_set<T> [with T=X]` is created by any means.

----

On the other hand, consider `allocator<T>`. Its skeleton looks extremely similar to `hash<T>`'s:

    template<class T>
    struct allocator;  // primary template provided by the library writer

    template<class T, class Alloc = allocator<T>>
    struct vector {
        // uses Alloc::allocate(int) (*note)
    };

    // provided by the programmer
    class X;

However, nobody would ever suggest that the programmer ought to specialize `allocator<X>`!
There is only one valid way to change the behavior of `vector<X>`: to define a new
allocator and use that allocator when creating the vector.

    // new type provided by the programmer
    template<class T> struct MyAllocator { ... };

    vector<X, MyAllocator<X>> v;  // Success!

There is a semantic difference between a "hasher of Xs" and an "allocator of Xs," despite
their syntactic similarity!

----

Staying on that same hand for a minute, consider `char_traits<T>`, again with the same skeleton:

    template<class T>
    struct char_traits;  // primary template provided by the library writer

    template<class T, class Traits = char_traits<T>>
    struct basic_string {
        // uses Traits::length, Traits::compare,...
    };

    // a new "character type" provided by the programmer
    class X;

Just as in the previous case: nobody would ever suggest that the programmer ought to
specialize `char_traits<X>`! There is only one valid way to change the behavior of
`basic_string<X>`: to define a new "character traits class" and use that class when
creating the string.

    // new type provided by the programmer
    struct CaseInsensitiveTraits { ... };

    basic_string<char, CaseInsensitiveTraits> s;  // Success!

Also in this boat: `regex_traits`.

----

But now, switch hands. I told a little white lie above.

    template<class T, class Alloc = allocator<T>>
    struct vector {
        // uses Alloc::allocate(int) (*note)
    };

This is wrong! What actually happens is this:

    template<class T, class Alloc = allocator<T>>
    struct vector {
        // uses allocator_traits<Alloc>::allocate(Alloc&, int)
    };

`allocator_traits` — aha, a new "`_traits`" class! But this one doesn't behave anything like `char_traits`
or `regex_traits` (which, you'll recall, both behave like `allocator`). This new
`allocator_traits` behaves more like the _façade_ design pattern. Its purpose is
to take a bare-bones implementation of the
[`Allocator` concept](https://www.youtube.com/watch?v=0MdSJsCTRkY) and flesh it out
by adding *sensible default implementations* of any missing members. So for example
`allocator_traits<A>::destroy(a, p)` will call `a.destroy(p)` if that member exists;
but otherwise it will sensibly default to just calling `p->~U()`.

And there are a *lot* of potentially missing members:
[20 by my count](https://en.cppreference.com/w/cpp/memory/allocator_traits#Member_types).

Notice that the name `allocator_traits` is hard-coded into the *implementation* of `vector`,
whereas the names `hash`, `char_traits`, `allocator`, and so on are hard-coded merely into
various *interfaces* (as defaults).  You can instantiate a `basic_string` that doesn't
use `std::char_traits`; you *cannot* instantiate a `basic_string` that doesn't use
`std::allocator_traits`.

We say that the name `allocator_traits` is *well-known*: it is baked into the library
implementation. You cannot parameterize it away.

And, again, the reason `allocator_traits` is there is purely to provide *sensible defaults*
for missing methods of the user's `Alloc` parameter. The user is not permitted to specialize
`allocator_traits` — it's not like `hash`. If the user tries, he'll hit two problems.
One is that it's just super tedious!

    template<class T>
    struct MyAlloc : std::allocator<T> {
        using std::allocator<T>::allocator;
        template<class U> struct rebind {
            using other = MyAlloc<U>;
        };
    };

    // Don't try this at home.
    template<class T>
    struct std::allocator_traits<MyAlloc<T>> {
        using A = MyAlloc<T>;
        using allocator_type = A;
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using void_pointer = void*;
        using const_void_pointer = const void*;
        using difference_type = std::ptrdiff_t;
        using size_type = std::size_t;
        using propagate_on_container_copy_assignment = std::true_type;
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_swap = std::true_type;
        using is_always_equal = std::true_type;
        template<class U> using rebind_alloc = MyAlloc<U>;
        template<class U> using rebind_traits = std::allocator_traits<MyAlloc<U>>;
        static T *allocate(A& a, size_t n) { return a.allocate(n); }
        static void deallocate(A& a, T *p, size_t n) { return a.deallocate(p, n); }
        template<class U, class... Args> static void construct(A&, U *p, Args&&... args) { ::new ((vo$
        template<class U> static void destroy(A&, U *p) { p->~U(); }
        static A select_on_container_copy_construction(const A& a) { return a; }
        static size_t max_size(const A&) { return size_t(-1); }
    };

    int main() {
        std::vector<int, MyAlloc<int>> vec;
        vec.push_back(1);
    }

The second problem is that the above ill-advised code *will not compile* under libc++, because libc++
(rightly) does not anticipate that any programmer would be so foolish as to write an
explicit specialization of `std::allocator_traits<X>`. Therefore libc++ feels little
compunction in [secretly upping the number of members from 20
to 23](https://github.com/llvm-mirror/libcxx/blob/54238057d6/include/memory#L1609-L1617):

    /usr/include/c++/v1/vector:889:21: error:
          no member named '__construct_backward' in 'std::__1::allocator_traits<MyAlloc<int> >'
        __alloc_traits::__construct_backward(this->__alloc(), this->__begin_, this->__end_, __...
                        ^
    /usr/include/c++/v1/vector:1575:5: note:
          in instantiation of member function 'std::__1::vector<int, MyAlloc<int>
          >::__swap_out_circular_buffer' requested here
        __swap_out_circular_buffer(__v);
        ^

libc++ (rightly) treats `allocator_traits` as its own personal playground, where it can stash helper methods
such as this `__construct_backward`. (Full disclosure: I do have
[a patch out](https://reviews.llvm.org/D49317) which aims to change this implementation
on code-cleanliness grounds. But I fully endorse the underlying notion that `allocator_traits`
is the library's own personal playground.)

[UPDATE, 2024-01-30: C++23 finally codifies this notion; user specializations of `allocator_traits`
are officially forbidden since [P2652](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2652r2.html).)

----

Another example in this category is `iterator_traits`.

    template<class T>
    struct iterator_traits;  // primary template provided by the library writer

    template<class Iter>
    auto distance(Iter first, Iter last) {
        // uses iterator_traits<Iter>::difference_type
    };

    // a new iterator type provided by the programmer
    class X;

As with `allocator_traits`, the name `iterator_traits` is *well-known* (baked into
the library's implementation, rather than the interface). And as with `allocator_traits`,
the programmer would be foolish to try to specialize it for their new type `X`.

With `iterator_traits` we see more clearly why traits classes are useful in template
programming. Whereas `vector` could easily have replaced `allocator_traits<Alloc>::allocate`
with direct calls to `Alloc::allocate`, our `distance` algorithm cannot directly refer to
`Iter::difference_type`. Why? Because `Iter` may not be a class type at all! It could be
a plain old native pointer. So the library writer *needs* to introduce a traits class
as a sort of coatrack on which to hang these members.

----

In the same vein, we have `pointer_traits` (as in fancy pointers, not as in smart pointers).

    template<class T>
    struct allocator;  // primary template provided by the library writer

    template<class T, class Alloc = allocator<T>>
    struct vector {
        using P = typename allocator_traits<Alloc>::pointer;
        // uses pointer_traits<P>::rebind, etc.
    };

    // provided by the programmer
    template<class T> class MyPtr { ... };
    template<class T> class MyAlloc {
        using pointer = MyPtr<T>;
    };

    vector<int, MyAlloc<int>> v;  // Success!

As with `allocator_traits` and `iterator_traits`, the name `pointer_traits` is *well-known* (baked into
the library's implementation, rather than the interface). And as with `allocator_traits` and `iterator_traits`,
the programmer would be foolish to try to specialize `pointer_traits` for their new type `MyPtr`.

As with `allocator_traits` and `iterator_traits`, `pointer_traits<P>` is a façade
that provides *sensible default implementations* of 5 members which are potentially
missing from `P`. For example, `pointer_traits<P>::pointer_to(T& r)` calls
`P::pointer_to(r)` if that member exists; but otherwise it will sensibly default to
SFINAEing away. `pointer_traits<P>::difference_type` is `P::difference_type` if that
member exists; but otherwise it will sensibly default to `ptrdiff_t`. And so on.

[EDIT: Glen Fernandes points out that existing implementations do _not_ SFINAE
away an omitted `pointer_to`; they just let the body go ill-formed, which is not SFINAE-friendly.
Modern fancy-pointer code should generally
[use `static_cast` for type conversions](https://www.youtube.com/watch?v=0MdSJsCTRkY)
anyway; but the SFINAE-unfriendliness of `pointer_to` is still more of a "bug"
than a "feature."]

Now for the very bad news!

In the C++2a working draft right now, there is a global function called
[`std::to_address(p)`](https://en.cppreference.com/w/cpp/memory/to_address). It is defined to
call [`pointer_traits<P>::to_address(p)`](https://en.cppreference.com/w/cpp/memory/pointer_traits/to_address)
if that member exists. <b>But who is supposed to make that member exist?</b> Not the primary
template — it does not define `to_address(p)` as `p.operator->()`, although it certainly could.
And not the user — the user would be extremely foolish to try to specialize `pointer_traits`
for their own types. So right now in the C++2a working draft, we've got a contradiction: the
user both *is* and *isn't* supposed to specialize `pointer_traits`. This is horrible, and I
hope someone submits a fix for it before 2020.

The fix is simple: just use the exact same pattern for `to_address` as we've already used
successfully for `pointer_to`.

The current, broken design was proposed in [P0653](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0653r2.html)
with a rationale that explicitly caters to users who do what they're not supposed to do:

> This preserves compatibility for any user specializations of `pointer_traits`
> that do not define a `to_address` member function.

"User specializations of `pointer_traits`" are exactly what we just said should never happen.
(And we showed, via libc++'s helpful example, that users who *try* to specialize well-known
façade classes earn only well-deserved pain.)

In P0653 author Glen Fernandes' defense, the broken design appears to have showed up only in the paper's
second revision, thanks to a bit of [design by committee](https://en.wikipedia.org/wiki/Camel).

----

So, we've now covered every "`_traits` class" in C++17!  We've discovered that they fall into
two general categories: convenient defaults baked into *interfaces* (`std::char_traits`,
`std::regex_traits`) and well-known façade classes baked
into *implementations* (`std::pointer_traits`, `std::iterator_traits`, `std::allocator_traits`).
We've discovered that some other (perhaps "most") convenient defaults don't have names
ending in "`_traits`" at all (`std::less`, `std::allocator`).

And then there's the example with which we opened this essay — `std::hash` — which seems to
be a little bit of both. It is certainly a convenient default baked only into interfaces.
Yet, at the same time, it is widely expected that the user *should* specialize `std::hash<X>`
for each user-defined class `X` — that this is how you make `X` "hashable" in a generic sense.
I cannot readily think of any other standard library entity that behaves like `std::hash`
in this respect... yet.

In ["The Best Type Traits C++ Doesn't Have"](https://www.youtube.com/watch?v=MWBfmmg8-Yo) (May 2018),
I describe a library template called `tombstone_traits`. In the presentation, I placed it
alongside `allocator_traits`, `iterator_traits`, and `pointer_traits` as a *well-known name*
that can have only one specialization per type `T`. However, I proposed that the user ought to
be able to specialize it for their own types:

    template<class T>
    struct tombstone_traits;  // primary template provided by the library writer

    template<class T>
    class optional {
        // uses tombstone_traits<T>::spare_representations, etc.
    };

    // specialization provided by the programmer
    class X;
    template<> struct tombstone_traits<X> { ... };

    optional<X> o;  // Success!

But, not long after that presentation, Nicole Mazzuca convinced me that it would
actually be much more natural to express it as a *convenient default*, which I now
realize is exactly isomorphic to `std::hash`:

    template<class T>
    struct default_tombstone_traits;  // primary template provided by the library writer

    template<class T, class Traits = default_tombstone_traits<T>>
    class optional {
        // uses Traits::spare_representations, etc.
    };

This can be used in either of two ways:

    // specialization provided by the programmer
    template<>
    struct default_tombstone_traits<X> { ... };

    optional<X> o;  // Success!

or:

    // new type provided by the programmer
    struct XUndertaker { ... };

    optional<X, XUndertaker> o;  // Success!

This `hash`-like design is more useful and flexible than the design presented in
my C++Now video. In particular, it allows doing things like

    using general_purpose_optional_int = optional<int>;  // 8 bytes
    using special_purpose_optional_int = optional<int, MinusOneAsTombstone>;  // 4 bytes

whereas in the C++Now model you had to pick one or the other to be the One True
`optional<int>`, and eschew the other entirely.

(Full disclosure: I am *not* pursuing `tombstone_traits` for standardization in
any form. I imagine no mainstream vendor would accept it because of ABI breakage.)

----

Speaking of "`_traits`" classes that *aren't* in C++17...

The [Coroutines TS](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/n4760.pdf)
(currently, and controversially, angling for inclusion in C++2a — I think it is woefully unready
for prime time) adds to the mix a new entity called `std::coroutine_traits<T>`, with *just one* member:

    // primary template provided by the library writer
    template<class T, class...>
    struct coroutine_traits {
        using promise_type = typename T::promise_type;
    };

    X foo(A a, B b) {
        co_await bar();
        // core language magic kicks in and...
        // uses coroutine_traits<X, A, B>::promise_type
    }

By looking at the primary template and the usage pattern, we can see that `coroutine_traits` fills the
same ecological niche as `allocator_traits`: it has a *well-known name* baked into the implementation,
and it provides a *façade* in front of the user-provided type `C`. There is nothing we can do by
specializing `coroutine_traits<MyCoro>` that we couldn't do more easily by putting members
directly into `MyCoro`.

And if the user does specialize `coroutine_traits<MyCoro>`, then he runs the risk of breakage —
either today (as in our libc++ example) or tomorrow (when C++2b adds additional members to the
primary template of `coroutine_traits`, and the user's C++2a-era specialization doesn't have those
members).

By the way, you might be wondering what's the purpose of the extra `class...` in that primary
template. I'll tell you: [I don't know.](https://www.youtube.com/watch?v=kDtabTufxao&t=2m13s)

----

Finally, already in the C++2a working draft, we find
[`std::chrono::zoned_traits`](https://en.cppreference.com/w/cpp/chrono/zoned_traits):

    // primary template provided by the library writer
    template<class P>
    struct zoned_traits;

    template<class D, class P = const std::chrono::time_zone*>
    class zoned_time {
        // uses zoned_traits<P>::default_zone
    };

Notice that this is almost exactly isomorphic to the allocator model:

| `zoned_traits`                             | `allocator_traits`              |
| `P`                                        | `A`                             |
| `const std::chrono::time_zone*`            | `allocator<T>`                  |
| `zoned_traits<P>::default_zone`            | `allocator_traits<A>::allocate` |
| `P::default_zone` (if it exists) `(*note)` | `A::allocate` (if it exists)    |

The glitch is that `zoned_traits<P>::default_zone` actually does *not* sensibly default
to calling `P::default_zone` if it exists. In fact, the primary template
`zoned_traits<P>` is *left undefined!*

This is a defect preliminary to the already-mentioned defect in `pointer_traits::to_address`:
`zoned_traits` seems almost designed to *encourage* user specializations, which is
exactly the problem that led to the camel-humped design of `pointer_traits::to_address`.
It would be better to nip the problem in the bud by requiring that users *never*
specialize `zoned_traits`, thus making it exactly analogous to the other well-known
façades `iterator_traits`, `allocator_traits`, and `pointer_traits`.

----

This essay has gone on long enough. Next time, perhaps I'll discuss the design of
`numeric_limits<T>`. It doesn't easily fit into any of our three categories.
Users would find it difficult to specialize its
[34 members](https://en.cppreference.com/w/cpp/types/numeric_limits#Member_constants) correctly.
Its name is well-known, but only in *extremely* obscure circles ([1](https://en.cppreference.com/w/cpp/memory/allocator/max_size),
[2](https://en.cppreference.com/w/cpp/chrono/duration_values/min), [3](https://en.cppreference.com/w/cpp/utility/compare/strong_order)).
My current take is that it is an ungainly artifact of C++'s early days — a coelacanth
that doesn't necessarily fit into *anyone's* taxonomy. But I haven't thought enough about it yet.

Until next time!
