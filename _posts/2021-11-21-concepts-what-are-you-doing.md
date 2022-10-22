---
layout: post
title: 'Is `path` convertible to `string_view`?: a war story'
date: 2021-11-21 00:01:00 +0000
tags:
  concepts
  llvm
  metaprogramming
  overload-resolution
  pitfalls
  war-stories
excerpt: |
  This story comes from [the libc++ review](https://reviews.llvm.org/D113161)
  implementing [P1989 "Range constructor for std::string_view 2: Constrain Harder"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1989r2.pdf)
  (Corentin Jabot, March 2021). That paper made `std::string_view`
  implicitly convertible-from basically any contiguous range of characters.
  Which, to be clear, is probably a good thing. But it turned up a really
  interesting interaction in libc++'s `filesystem::path`. Somehow,
  enabling this constructor template in the `<string_view>` header
  caused libc++'s `filesystem::path` to stop being a `range`!
---

{% raw %}
This story comes from [the libc++ review](https://reviews.llvm.org/D113161)
implementing [P1989 "Range constructor for std::string_view 2: Constrain Harder"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1989r2.pdf)
(Corentin Jabot, March 2021). That paper made `std::string_view`
implicitly convertible-from basically any contiguous range of characters.
Which, to be clear, is probably a good thing. But it turned up a really
interesting interaction in libc++'s `filesystem::path`. Somehow,
enabling this constructor template in the `<string_view>` header
caused libc++'s `filesystem::path` to stop being a `range`!

    // Before the patch
    #include <filesystem>
    #include <ranges>
    static_assert(std::ranges::range<std::filesystem::path>);
    static_assert(std::ranges::range<const std::filesystem::path>);

    // After the patch
    #include <filesystem>
    #include <ranges>
    static_assert(std::ranges::range<std::filesystem::path>);        // OK
    static_assert(!std::ranges::range<const std::filesystem::path>); // Wat

I'll skip over the investigation and jump to the punch line.
Consider the following translation unit ([Godbolt](https://godbolt.org/z/55hYTzW7q)):

    #define ENABLE_FORWARD_RANGE 1

    struct StringView {
        StringView(std::ranges::contiguous_range auto r);
    };

    struct Path {
        struct Iterator;
        Iterator begin() const;
        Iterator end() const;

        void internalDetail(void*, StringView) const;
        void internalDetail(int, const Path&) const;

        void foo() const {
            internalDetail(ENABLE_FORWARD_RANGE, *this);
        }
    };

    struct Path::Iterator {
        using value_type = char;
        using difference_type = int;
        const char& operator*() const;
        Iterator& operator++();
        Iterator operator++(int);
        friend bool operator==(Iterator, Iterator);
    };
    static_assert(std::forward_iterator<Path::Iterator>);

    static_assert(!std::convertible_to<Path, StringView>);
    static_assert(std::ranges::forward_range<Path>);

Observe that `Path::Iterator` is a forward iterator. Observe
that `Path` is a forward range (because its `begin` and `end`
return forward iterators). Observe that `Path` is _not_ convertible
to `StringView` (because although it's a forward range, it's
not a contiguous range).

But now, let's change the macro at the top of the code:

    #define ENABLE_FORWARD_RANGE 0

`Path::Iterator` remains a forward iterator, but now
our final assertion fails: `Path` itself is no longer
a forward range!

    static_assert(std::ranges::forward_range<Path>);  // fails!

What happened here?


## The explanation

The answer involves overload resolution on that call to
`internalDetail`.

    void internalDetail(void*, StringView) const; // #1
    void internalDetail(int, const Path&) const;  // #2

    void foo() const {
        internalDetail(ENABLE_FORWARD_RANGE, *this);
    }

When `ENABLE_FORWARD_RANGE` is `1`, `internalDetail`
overload #1 is knocked out of contention immediately,
because the caller's first argument (`1`) is not convertible to
this overload's first parameter type (`void*`). That leaves
only overload #2, which is viable, so it gets used.
Nothing weird is happening in this case.

But when `ENABLE_FORWARD_RANGE` is `0`, the compiler
sees that `0` is convertible to `void*` (because it's
a null pointer constant). So the compiler must look at
the second argument (`*this`) and decide whether
it's convertible to the overload's second parameter
type (`StringView`). So we look at `StringView`'s
constructor template. It works only for contiguous ranges.
Does `Path` satisfy `contiguous_range`? Well, no:

* `contiguous_range<Path>` subsumes `range<Path>`
* `range<Path>` requires `path.begin()`'s return type to satisfy `input_or_output_iterator`
* `Path::Iterator` at this point is an incomplete type

and an incomplete type cannot satisfy `input_or_output_iterator`.
So that leaves only overload #2, which is viable,
so it gets used.

It seems as if the same thing happened in both cases, doesn't it?
But in the `ENABLE_FORWARD_RANGE=0` case, the overload resolution
had a side effect: it evaluated `range<Path>` to `false`! So,
later on in the translation unit, when we ask whether `Path`
satisfies `forward_range`:

* `forward_range<Path>` subsumes `range<Path>`
* `range<Path>` is already known to be `false`

Of course we humans know that `range<Path>` is _actually_ true,
but the compiler has [memoized](https://en.wikipedia.org/wiki/Memoization)
the `false` result from earlier, and nothing we do at this point
will ever convince it to re-evaluate that belief.

In the `ENABLE_FORWARD_RANGE=1` case, we never tricked the
compiler into memoizing the wrong value for `range<Path>`,
and so `forward_range<Path>` correctly yields `true`.

Notice that the value of `convertible_to<Path, StringView>` is
invariably `false` throughout the whole translation unit. The problem
is that evaluating that `false` claim causes the compiler to pre-commit
to the falseness of other things that later turn out to be true.


## The (code) solution

The solution to libc++'s `path` problem was simply to change some internal
details from

    __lhs->compare(__rhs)  // overloaded for string_view and path

to

    __lhs->__compare(__rhs.__pn_)  // only for string_view

This is a specific application of two of my general mantras:

* [Don't give two things the same name](https://www.youtube.com/watch?v=OQgFEkgKx2s)
    without a good reason. (In libc++, `path::compare` is overloaded,
    like the toy example's `Path::internalDetail`.
    `path::__compare` is non-overloaded, so the compiler doesn't have to do
    as much work. It turns out that some of the work it gets to skip is work
    that was actively harmful to us.)

* [Explicit is better than implicit.](https://www.python.org/dev/peps/pep-0020/)
    (We're actually still implicitly converting `string __pn_` to
    `string_view` here, but at least we've removed _one_ layer of implicitness;
    and that turns out to be the layer that matters.)


## The (standardization) solution

GCC wisely emits an error message whenever it detects that a concept's truth value
has changed over the course of a translation unit. Like most template error messages,
it's really messy (large swaths redacted here); but it does get the point across
eventually.

    bits/ranges_base.h: In substitution of 'template<class _Tp>
      requires (__maybe_borrowed_range<_Tp>) && ((is_array_v<typename std::remove_reference<_Tp>::type>)
            || (__member_begin<_Tp>) || (__adl_begin<_Tp>))
      constexpr auto std::ranges::__cust_access::_Begin::operator()(_Tp&&) const
      [with _Tp = Path&]':
    [...]
    bits/iterator_concepts.h:945:32: error: satisfaction value of atomic constraint
      'requires(_Tp& __t) {{std::ranges::__cust_access::__decay_copy(__t->begin())}
      -> decltype(auto) [requires std::input_or_output_iterator<<placeholder>, >];}
      [with _Tp = Path&]' changed from 'false' to 'true'
      945 |       concept __member_begin = requires(_Tp& __t)
      946 |         {
      947 |           { __decay_copy(__t.begin()) } -> input_or_output_iterator;
      948 |         };
    bits/ranges_base.h:581:22: note: satisfaction value first evaluated to 'false' from here
      581 |         ranges::begin(__t);

I'd like to see Clang and MSVC gain similar error messages; and in fact I'd like
the C++ Standard itself to specify that any constraint _shall_ produce
the same truth value every time it's evaluated for the same inputs, or else the
program is ill-formed and the compiler must produce a diagnostic. (That is,
I'd like WG21 to standardize GCC's behavior here and force the other vendors
to follow suit.)

Violations of this rule that span translation units — for example if `a.cpp` includes
an extra specialization of `enable_view`, such that `std::ranges::view<Path>`
is `true` in `a.o` but `false` in `b.o` — would naturally continue to be
[IFNDR](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#ifndr).

Philosophical aside: It's pretty weird that the static properties of types
can _change_ over the course of compilation; it kind of subverts the entire
point of static typing. I wish C++20 Concepts didn't suffer from this issue
at all. But that ship has certainly sailed, and besides, I don't know what
the fix would have been, other than the ill-formedness I propose above.

For now, my advice for working programmers continues to be:

> Don't use constrained templates for everyday stuff.

I used to say that simply because constrained templates are slower to compile
than unconstrained ones, and tend to give worse error messages (because when
misused they'll quietly drop out of overload resolution instead of giving
you an error message on the exact line that fails). But now there's this
additional reason: Every call to a constrained template has side effects,
and _can_ cause the compiler to memoize the wrong truth value for a constraint.
Ninety-nine times out of a hundred, it _won't_ cause the compiler to memoize
anything wrong; but it's just one more arcane subtlety for the working programmer
to worry about. Drop the constraints and save yourself some brain cells.

In libc++ we have to use constrained templates because the Standard mandates it;
but in code that _you_ control, please use them sparingly, if at all!
And specifically beware of calling constrained templates from within inline member
functions, or from header files, or anywhere else where a relevant class type
might be "not quite complete yet."
{% endraw %}
