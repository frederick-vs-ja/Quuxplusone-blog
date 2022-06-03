---
layout: post
title: "C++20's parenthesized aggregate initialization has some downsides"
date: 2022-06-03 00:01:00 +0000
tags:
  c++-learner-track
  constructors
  initialization
  pitfalls
  slack
excerpt: |
  Somehow the topic of [P0960 parenthesized aggregate initialization](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0960r3.html)
  has come up three times in the past week over on [the cpplang Slack](https://cppalliance.org/slack/).
  The good news is that usually the asker is curious why some reasonable-looking C++20 code
  fails to compile in C++17 — indicating that C++20's new rules are arguably _more_ intuitive
  than C++17's.

  But let's start at the beginning.
---

{% raw %}
Somehow the topic of [P0960 parenthesized aggregate initialization](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0960r3.html)
has come up three times in the past week over on [the cpplang Slack](https://cppalliance.org/slack/).
The good news is that usually the asker is curious why some reasonable-looking C++20 code
fails to compile in C++17 — indicating that C++20's new rules are arguably _more_ intuitive
than C++17's.

But let's start at the beginning.


## The basics of brace-initialization versus parens-initialization

Regular readers of this blog may remember my simple initialization guidelines from
["The Knightmare of Initialization in C++"](/blog/2019/02/18/knightmare-of-initialization/) (2019-02-18):

* Use `=` whenever you can.

* Use initializer-list syntax `{}` only for element initializers (of containers and aggregates).

* Use function-call syntax `()` to call a constructor, viewed as an [object-factory](/blog/2018/06/21/factory-vs-conversion/).

And one more rule:

* Every constructor should be `explicit`, unless you mean it to be usable as an implicit conversion.

So for example we can write

    int a[] = {1, 2};
    std::array<int, 2> b = {1, 2};
    std::pair<int, int> p = {1, 2};

    struct Coord { int x, y; };
    struct BadGrid { BadGrid(int width, int height); };
    struct GoodGrid { explicit GoodGrid(int width, int height); };

    Coord c = {1, 2};
    auto g1 = BadGrid(10, 20);
    auto g2 = GoodGrid(10, 20);

Notice that even when the _type author_ of `BadGrid` has broken our rule that all constructors should
be `explicit`, it's still possible for us as the _client-code author_ to ignore that "mistake" and
use function-call syntax `()` anyway. It would have been physically possible for us to write

    BadGrid g1 = {10, 20};  // a grid with dimensions 10x20

but that would have been a solecism according to the guidelines I just laid out: The resulting
`BadGrid` is not _conceptually_ a sequence of two elements `{10, 20}` in the same way that an
array, pair, or `Coord` is _conceptually_ a sequence of two elements. Therefore, even though
we _can_ implicitly convert the braced initializer list `{10, 20}` to a `BadGrid`, we _should_ not.

This guideline is particularly relevant when dealing with STL containers like `vector`,
for three reasons:

- Every STL container does represent a sequence of elements.

- Every STL container has an absolutely massive constructor overload set.

- For historical reasons (with which I disagree), STL types deliberately make _almost all_ of their
    constructors non-`explicit`.

Consider the following four initializations of `vector`:

    std::vector<int> v1 = {10, 20};
    std::vector<std::string> v2 = {"x", "y"};
    auto v3 = std::vector<int>(10, 20);
    auto v4 = std::vector<std::string>(10, "y");

These are all appropriate initializations according to my guidelines. The first two depict
initializing a `vector` with a sequence of elements: `{10, 20}` or `{"x", "y"}`. In both cases,
the resulting vector ends up with 2 elements. The second two initialize a `vector` using one
of its many "object-factory" constructors: specifically, the one that takes a size and a
fill value. `v3` is initialized with 10 copies of `20`; `v4` is initialized with 10 copies of `"y"`.
In case `v4`, we physically _could_ have written

    std::vector<std::string> v4 = {10, "y"};  // Faux pas!

but according to my guidelines we _should_ not write that, for the simple reason that we're
not intending to create a vector with the sequence of elements `{10, "y"}`. This situation is
exactly analogous to the `BadGrid g1` case above.


## Aggregate versus non-aggregate initialization

The next thing you should know about initialization in C++ is that C++ treats aggregates
differently from non-aggregates. In C++98, this distinction
was extremely visible, because curly braces could be used only to initialize _aggregates_,
by which I mean plain old C-style structs and arrays:

    Coord ca = Coord(1, 2);  // syntax error in C++98
    Coord cb = {1, 2};       // OK

    BadGrid ga = BadGrid(1, 2);  // OK
    BadGrid gb = {1, 2};         // syntax error in C++98

    std::pair<int, int> pa(1, 2);     // OK
    std::pair<int, int> pb = {1, 2};  // syntax error in C++98

In C++11 we got _uniform initialization_, which let us use `{}` to call constructors,
like this:

    Coord ca = Coord(1, 2);  // still a syntax error in C++11
    Coord cb = {1, 2};       // OK

    BadGrid ga = BadGrid(1, 2);  // OK
    BadGrid gb = {1, 2};         // OK since C++11 (but solecism)

    std::pair<int, int> pa(1, 2);     // OK (but solecism)
    std::pair<int, int> pb = {1, 2};  // OK since C++11 (in fact, preferred)

So in C++11 we have two different meanings for `{}`-initialization: Sometimes it means
we're calling a constructor with a certain set of arguments (or maybe with a `std::initializer_list`),
and sometimes it means we're initializing the members of an aggregate. The second case,
_aggregate initialization_, has several special powers:

<b>First,</b> aggregate initialization initializes each member _directly._ When you call
a constructor, the arguments are matched up to the types of the constructor parameters;
when you do aggregate initialization, the initializers are matched up directly to the types
of the object's data members. This matters mainly for immovable types like `lock_guard`:

    struct Agg {
        std::lock_guard<std::mutex> lk_;
    };
    struct NonAgg {
        std::lock_guard<std::mutex> lk_;
        NonAgg(std::lock_guard<std::mutex> lk) : lk_(lk) {}
    };

    std::mutex m;
    Agg a = { std::lock_guard(m) };      // OK
    NonAgg na = { std::lock_guard(m) };  // oops, error

In the snippet above, the prvalue `std::lock_guard(m)` directly initializes
`a.lk_`; but on the next line, the prvalue `std::lock_guard(m)` initializes
only the constructor parameter `lk` — there's no way for the author of that constructor
to get `lk`'s value into the data member `na.lk_`, because `lock_guard` is immovable.

Even for movable types, the direct-initialization of aggregates can be a performance benefit.
Recall from ["The surprisingly high cost of static-lifetime constructors"](/blog/2018/06/26/cost-of-static-lifetime-constructors/) (2018-06-26)
that `std::initializer_list` is a view onto an immutable array. So:

    std::string a[] = {"a", "b", "c"};

direct-initializes three `std::string` objects in an array `a`, whereas

    std::vector<std::string> v = {"a", "b", "c"};

direct-initializes three `std::string` objects in an anonymous array,
creates an `initializer_list<string>` referring to that array, and then
calls `vector`'s constructor, which makes _copies_ of those `std::string`s.
Or again,

    std::array<std::string, 3> b = {"a"s, "b"s, "c"s};

direct-initializes three `std::string` objects into the elements of `b`
(which the Standard guarantees is an aggregate type), whereas

    using S = std::string;
    std::tuple<S, S, S> t = {"a"s, "b"s, "c"s};

direct-initializes three temporary `std::string` objects on the stack,
and then calls `tuple`'s constructor with three `std::string&&`s.
That constructor _move-constructs_ into the elements of `t`
and finally destroys the original temporaries.

<b>Second,</b> for backward-compatibility with C, aggregate initialization
will [value-initialize](https://eel.is/c++draft/dcl.init.general#def:value-initialization)
any trailing data members.

    struct sockaddr_in {
        short          sin_family;
        unsigned short sin_port;
        struct in_addr sin_addr;
        char           sin_zero[8];
    };

    sockaddr_in s = {AF_INET};
    assert(s.sin_port == 0);
    assert(s.sin_addr.s_addr == 0);
    assert(s.sin_zero[7] == 0);

This is useful for C compatibility, but it does lead to some surprising results:
whether it's "OK" to omit the initializers of trailing elements depends on whether
the type being initialized is an aggregate or not, even though we're uniformly using
braced-initializer-list initialization _syntax_ in both cases.

    Coord c = {42};    // OK: Coord is an aggregate
    BadGrid g = {42};  // ill-formed: BadGrid is not an aggregate

    std::array<int,3> b = {1, 2};       // OK: array is an aggregate
    std::tuple<int,int,int> t = {1, 2}; // ill-formed: tuple is not an aggregate


## The coincidence of zero-argument initialization

Because C++11 introduced uniform initialization, `T{}` is permitted to call `T`'s default
constructor if it has one; or, if `T` is an aggregate, then `T{}` will do aggregate
initialization where every member is value-initialized.

    using B = std::array<int,3>;
    using T = std::tuple<int,int,int>;

    B b = B{};  // value-initialize, i.e., {0,0,0}
    T t = T{};  // call the zero-argument constructor, i.e., {0,0,0}

Also, ever since C++98, there's been [wording](https://timsong-cpp.github.io/cppwp/n3337/expr.type.conv#2)
to make `T()` do value-initialization as a special case.
(Sure, it looks like we're calling `T`'s zero-argument
constructor — and you can totally think of it that way in practice — but _technically_
this is a special-case syntax for
[_value-initializing_](https://eel.is/c++draft/dcl.init.general#def:value-initialization)
`T`, which _in turn_ calls `T`'s default constructor if it has one, but otherwise
recursively value-initializes `T`'s members.)

    B b = B();  // value-initialize, i.e., {0,0,0}
    T t = T();  // value-initialize, i.e. call the default constructor, i.e., {0,0,0}


## `emplace_back` presents a stumbling block

C++11 also introduced the idea of perfect-forwarding arguments through APIs like
`emplace_back`. The idea of `emplace_back` is that you pass in some arguments
`Args&&... args`, and then the vector will construct its new element using a
placement-new expression like `::new (p) T(std::forward<Args>(args)...)`.
Notice the parentheses there — not braces! This is important because we want to
ensure that we get consistent behavior when emplacing an STL container.
Compare the following snippet with our first section's `v3`/`v4` example:

    std::vector<std::vector<int>> vvi;
    vvi.emplace_back(10, 20);  // emplace vector<int>(10, 20)

    std::vector<std::vector<std::string>> vvs;
    vvs.emplace_back(10, "y"); // emplace vector<string>(10, "y")

But now, consider this C++17 code:

    using T = std::tuple<int,int,int>; // non-aggregate
    using B = std::array<int,3>;       // aggregate

    std::vector<T> vt; // vector of non-aggregates
    std::vector<B> vb; // vector of aggregates

    vt.emplace_back(); // OK, emplaces T()
    vb.emplace_back(); // 1: OK, emplaces B()

    vt.emplace_back(1,2,3); // OK, emplaces T(1,2,3)
    vb.emplace_back(1,2,3); // 2: Error in C++17!

Line `1` emplaces an `array` object constructed with `B()`, which, as we saw in the
preceding section, means value-initialization: the same as `{0,0,0}`.

Line `2` attempts to emplace an `array` constructed with `B(1,2,3)` — and this fails,
because `B` has no constructor taking three ints!

Whether a given `emplace_back` is legal depends on whether
the type being initialized is an aggregate or not, even though we're using
the same `emplace_back` _syntax_ in both cases.


## C++20's solution: Parens-init for aggregates

C++20 addressed the above `emplace_back` quirk by saying: well, if the problem is
that `B(1,2,3)` isn't legal syntax, let's just make it legal! C++20 adopted
[P0960 "Allow initializing aggregates from a parenthesized list of values,"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0960r3.html)
which extends the rules of initialization to cover the case where
"the destination type is a (possibly cv-qualified) aggregate class `A` and the
initializer is a parenthesized _expression-list_." In that case, initialization
proceeds just as in the curly-brace case, omitting a few minor quirks that are
triggered (since C++11) by the curly-brace syntax specifically:

- Curly-braced initializers are evaluated strictly left-to-right; parenthesized
    initializers can be evaluated in any order.

- Curly-braced initializers forbid narrowing conversions (such as `double`-to-`int`);
    parenthesized initializers do not.

- Curly-braced prvalues bound to reference data members can be lifetime-extended;
     parenthesized prvalues are never lifetime-extended.

- Curly-braced initialization lists can involve brace elision (e.g. depicting
    `{1, {2, 3}, 4}` as `{1, 2, 3, 4}`); parenthesized initialization lists cannot.

- Curly-braced initialization lists can include C++20 [designated initializers](https://eel.is/c++draft/dcl.init.general#nt:designated-initializer-list);
    parenthesized initialization lists cannot.

The result is that the following is legal C++20 (but not legal C++17):

    struct Coord { int x, y; }; // aggregate
    std::vector<Coord> vc;
    vc.emplace_back();        // OK since C++11, emplaces Coord() i.e. {0, 0}
    vc.emplace_back(10, 20);  // OK since C++20, emplaces Coord(10, 20) i.e. {10, 20}


## The unintended consequences

This being C++, naturally there are some rough edges and pitfalls.
I count at least three.

<b>First,</b> remember that even though `vc.emplace_back(10,20)` emplaces
the equivalent of `{10,20}`, it doesn't _actually_ use curly braces!
`emplace_back` still uses the round-parens syntax `Coord(10,20)`, and simply relies
on aggregate parens-init to convert that into the equivalent of `{10,20}`.
For non-aggregate types, where `T(x,y)` and `T{x,y}` do different things,
you're still going to get the `T(x,y)` behavior!

    std::vector<std::vector<int>> vvi;
    vvi.emplace_back(10, 20);  // OK since C++11, emplaces vector<int>(10, 20)
    assert(vvi[0].size() == 10);  // not 2

<b>Second,</b> look back at that list of curly-brace quirks ignored by aggregate parens-init.
Notice one quirk that's _not_ on that list: both kinds of aggregate initialization
are allowed to omit trailing initializers!

    Coord c1 = {10};       // OK since C++98, equivalent to {10, 0}
    Coord c2 = Coord(10);  // OK since C++20, equivalent to {10, 0}
    vc.emplace_back(10);   // OK since C++20, emplaces Coord(10) i.e. {10, 0}

In fact, since `Coord(10)` is legal in C++20, we can even write

    auto c3 = static_cast<Coord>(10);  // OK since C++20, equivalent to {10, 0}

which strikes me as probably terrible.

Vice versa, even though C++20 made it legal to aggregate-initialize array types
with parentheses in general, some of the syntactic space was deliberately reserved
for future standardization. For example, parens-init works for array variables
but not array prvalues:

    using A = int[3];
    A a1(1,2,3);         // OK since C++20
    auto a2 = A(1,2,3);  // still an error!

<b>Third,</b> probably the most annoying pitfall (although it does satisfyingly confirm
[my prejudice against `std::array`](/blog/2020/08/06/array-size/)): parens-init
is basically useless for `std::array`!

    using T = std::tuple<int, int, int>; // non-aggregate
    using B = std::array<int, 3>; // aggregate
    std::vector<T> vt;
    std::vector<B> vb;
    vt.push_back({1,2,3});  // OK since C++11
    vb.push_back({1,2,3});  // OK since C++11

    vt.emplace_back(1,2,3); // OK since C++11
    vb.emplace_back(1,2,3); // still an error!

The reason is that `B(1,2,3)` remains ill-formed even in C++20; and the reason for
_that_ is the fourth quirk in the list above: parenthesized initializer lists don't
do brace elision the way braced initializer lists do! `std::array<T,N>` is
[guaranteed](https://eel.is/c++draft/array.overview#2)
to be an aggregate initializable with `N` elements of type `T`, but nothing
about the data's underlying "shape" is guaranteed: does it have `N` data members of
type `T`? or one data member of type `T[N]`? or `N/2` data members of type `T[2]`?
Brace elision allows us not to care about such details when we use curly-braced
aggregate initialization, but not when we use parenthesized aggregate initialization.

    B b1(1,2,3);              // still an error! (technically implementation-defined)
    B b2{{1,2,3}};            // OK since C++11; direct-initializes (technically implementation-defined)
    B b3({1,2,3});            // OK since C++11; move-constructs
    vb.emplace_back({1,2,3}); // still an error!

The above `b3` is already legal in C++11; it represents direct-initialization of a
`B` from `{1,2,3}` (which overload resolution will decide to treat as `B{1,2,3}`),
and C++20 doesn't change this.
And `vb.emplace_back({1,2,3})` remains ill-formed, because `emplace_back`'s perfect
forwarding wants its `Args&&...` to have nicely deducible types, and a
brace-enclosed initializer list like `{1,2,3}` has no type.


## The bottom line

C++20's parenthesized aggregate initialization solves the tipmost link in a rather
long chain of unintended consequences:

- Aggregate types are quietly treated differently from non-aggregates; for example,
    they get more-direct initialization.

- `emplace_back` uses parens-initialization rather than brace-initialization,
    and we can't change that without breaking `vvi.emplace_back(10, 20)`.

- Therefore `vc.emplace_back(10, 20)` wants to parens-initialize `Coord(10, 20)`;
    in C++17 that's ill-formed unless we add a constructor `Coord(int, int)`.

- But giving a type a constructor makes it a non-aggregate, which means it loses
    the performance benefits of aggregate initialization. (Not that `Coord` cares
    about those benefits.)

- C++20 makes `vc.emplace_back(10, 20)` legal, by permitting `Coord(10, 20)` to
    do aggregate initialization just like `Coord{10, 20}`.

But as a side effect, C++20 makes `static_cast<Coord>(10)` legal (surprise!);
and even after all this rigmarole, C++20 fails to make either `vb.emplace_back(1,2,3)` or
`vb.emplace_back({1,2,3})` work as expected. That's bad.

But I'll leave you by reiterating the good news: When I've seen people bring up this
topic on [Slack](https://cppalliance.org/slack/), they're usually not at all surprised that
both of these lines

    std::vector<Coord> vc;
    vc.push_back({10, 20});  // A, OK since C++11
    vc.emplace_back(10, 20); // B, OK since C++20

both work in C++20. Generally, they're surprised only that line `B` _failed_ to work
_before_ C++20! So, in at least that narrow sense, C++20's parenthesized aggregate initialization
has improved the story for newcomers to C++.
{% endraw %}
