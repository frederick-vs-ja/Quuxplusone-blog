---
layout: post
title: 'What is the "vector pessimization"?'
date: 2022-08-26 00:01:00 +0000
tags:
  c++-learner-track
  exception-handling
  move-semantics
  pitfalls
  stl-classic
---

I always say that you should mark your move constructors `noexcept`,
or else you get _the vector pessimization._ But I haven't had a
blog post explicitly on the topic... until now.

## Part 1: Geometric resizing

Recall that `std::vector<T>` is a dynamically allocated (resizeable)
array: it heap-allocates enough space for a bunch of contiguous `T`s,
and exposes both its current `.size()` and its current total
`.capacity()` (which may at any given time be greater than
its size, and usually is). When you `.push_back` a new element
onto the end of a vector `v`, `v` won't need to allocate any more
storage _unless_ `v.size() == v.capacity()`.

Furthermore, whenever `v` does reallocate its buffer of `T`s, it
always allocates twice as much as it needs: If its old size was
16 elements, it'll allocate capacity for 32 elements. If its
old size was 32, it'll allocate 64. And so on.

> Actually ([Godbolt](https://godbolt.org/z/Exc6W36Ke)), each
> major library vendor does this slightly differently.
> GNU libstdc++ uses the algorithm above; libc++ bases its math on
> the old _capacity_ rather than the old size; and Microsoft's STL
> uses a growth factor of 1.5 instead of 2. But the result
> is effectively the same.

Each time `v` reallocates its buffer, it spends more and more time
copying data from the old buffer into the new buffer. But, because
capacity keeps increasing in a geometric progression, it takes longer
and longer to reach the next reallocation. These two factors balance
each other out, and we say that `v.push_back` takes constant time
_on average_ — it is "amortized constant time" or "amortized $$O(1)$$."

## Part 2: The strong exception guarantee

Let's look closer at that buffer-reallocating operation.
Its job is to allocate a new buffer, transfer the data from the old buffer
to the new buffer, and then repoint `v`'s data pointer at the new buffer
and deallocate the old one.

![Reallocation, via T's copy constructor](/blog/images/2022-08-26-realloc-via-copy.png)

`std::vector` is part of the C++98 STL, and C++98 didn't know anything
about move semantics. So of course the "transfer" step here, in the original
C++98 `vector`, was always done using `T`'s copy constructor. Each `T` in the old buffer
was copied down into the new buffer; then, the pointer was repointed (as already shown
in the diagram above), the old `T`s were destroyed, and the old buffer was deallocated.

This operation has the nice property of providing the [strong exception guarantee](https://www.boost.org/community/exception_safety.html).
There are several points here where an exception might be thrown: The memory allocation
itself might throw. Any one of the copy operations might throw. But, if any step threw
an exception, we could simply rewind: destroy any already-constructed copies, deallocate
the new buffer, keep `v`'s data pointer pointed at the old buffer. The old buffer still
contains all the user's old data, because our making copies of that data didn't modify it.
So we have the strong exception guarantee: If `push_back` throws an exception, then all
your data is still there; it's as if the `push_back` operation never happened.

## Part 3: Move semantics change the picture

C++11 gave us the move constructor as an optimization of the copy constructor. The
move constructor `T(T&&)` takes an rvalue reference to a `T` whose useful lifetime is
over, and can _move out of_ that `T`, cheaply pilfering its data members instead of
expensively making disjoint copies of them.

During vector reallocation, the `T`s in the vector's old buffer are essentially at the
end of their useful lifetimes. If the copy succeeds, we're just going to destroy them.
So, you might think we could have the vector _move out of_ those `T`s, like this:

![Reallocation, via T's move constructor](/blog/images/2022-08-26-realloc-via-move.png)

Here, the green boxes indicate `T` objects containing the user's data, and red boxes
indicate `T` objects that (still exist, but) have been pilfered of their contents and
left in a "moved-from state." This way clearly wastes less time expensively copying
the user's data!

But using move instead of copy causes us to lose the strong exception guarantee.
Suppose an exception is thrown (by `T`'s move constructor) at the point illustrated in the
diagram — during the move of the second or third element. We'd like to undo the operation
and roll back to the user's original vector, but we can't: we've already moved out of the
first two elements! If we destroy and deallocate the new buffer at this point, the user
will get back only _some_ of their original data.

> Could we use `swap` to get the user's green data back into the old buffer?
> In theory, `swap` never needs to throw. But in practice, no, because C++ doesn't treat
> `swap` as a primitive operation. If `T`'s move constructor throws, then you can
> bet `std::swap<T>` will also throw, because `std::swap<T>` is implemented in terms
> of move.

Notice that this lack-of-rollback is a problem only if `T(T&&)` throws an exception.
If `vector` had some way of assuring itself that `T(T&&)` would _never_ throw, then
we'd never need to roll back, and so it would be perfectly safe to use `T(T&&)` instead
of `T(const T&)`.

> Or, if we knew that `T` was trivially relocatable, we could use [relocation](https://www.youtube.com/watch?v=SGdfPextuAU&t=80s);
> but again, C++ doesn't currently understand relocation as a primitive operation.

## Part 4: Move-constructors that throw

You might say: "Why didn't C++11 just mandate that move-constructors must never throw?
We could have made non-throwing-ness part of what it means to be a move-constructor.
After all, writing a move constructor is optional. If you can't write a non-throwing
move constructor, just don't write one at all!" But this doesn't work either.

Consider `std::list<int>`. It's a heap-allocated linked list, where `lst.begin()`
gives you an iterator into the first heap-allocated node of the list, and `lst.end()` gives
you a one-past-the-end iterator to the _last-plus-one'th_ node of the list. There are
two practical approaches as to where that _last-plus-one'th_ node is stored. libstdc++ and libc++ 
store a "dummy node" inside the memory footprint of the `std::list` object itself. But Microsoft's STL
(based on Dinkumware's) stores an extra "sentinel node" out there on the heap with the rest of
the list's nodes. So, on MSVC, both `std::list`'s default constructor and its move constructor
need to allocate a sentinel node, and that memory allocation might (hypothetically) throw
an exception.

"So? If `std::list` can't be nothrow movable, maybe it shouldn't be movable at all."
Well, suppose Microsoft's `std::list` didn't define any move constructor.
You'd still be able to write `auto lst2 = std::move(lst);` — it would just end up calling
the copy constructor instead.
(See ["'Universal reference' or 'forwarding reference'?"](/blog/2022/02/02/look-what-they-need/) (2022-02-02).)
That was kind of the original point of treating the move constructor as merely another
overload of the copy constructor.

Furthermore, we really do _want_ `std::list` to have a move constructor. Move-constructing
a 100-element `std::list` might require heap-allocating one sentinel node, but copy-constructing
that 100-element `std::list` would require heap-allocating 101 nodes! Move semantics are still
a huge win, even if we sometimes have to make them potentially throwing.

"Could we just force Microsoft to rewrite their `std::list` implementation to match libstdc++
and libc++?" In theory, yes. In practice, no, a big important vendor isn't going to break their
ABI just because you asked nicely. (Also remember this was circa 2003–2009 we're talking about,
not 2022.)

Okay, so, C++11 really did need to admit the possibility of throwing move constructors.
So `vector`'s reallocation operation needed a way to distinguish throwing from non-throwing
move constructors. Basically `vector` needed to do this:

    if constexpr (is_nothrow_move_constructible_v<T>) {
        // move in a loop
    } else {
        // copy in a loop, and roll back on exception
    }

Implementing `is_nothrow_move_constructible` was a problem.
How do we solve a problem in C++? We add a keyword.

## Part 5: `noexcept`

The solution adopted in C++11 was to introduce a new keyword: `noexcept`.
Functions not marked `noexcept` (i.e., most functions) can throw exceptions.
Functions marked `noexcept` promise that they'll never throw exceptions.
In fact, it's not only a promise: it's also a firewall. Exceptions thrown from
lower levels will never propagate out past a `noexcept`; they'll slam into it
and `std::terminate` the program.

So now `vector`'s reallocation operation can just ask whether `T(T&&)` is `noexcept`.
If it is, then vector reallocation will use move-construction and be efficient.
If not, then vector reallocation will use copy-construction and be just as slow
as C++98 — but preserve the strong exception guarantee, in the case that an exception
ever actually _is_ thrown.

`noexcept` is propagated in the natural way by (implicitly or explicitly)
defaulted members:

    struct A { A(A&&); };
    struct B { B(B&&) noexcept; };
    struct JustB { B b; JustB(JustB&&) = default; };
    struct AB { A a; B b; };

    static_assert(!std::is_nothrow_move_constructible_v<A>);
    static_assert(std::is_nothrow_move_constructible_v<B>);
    static_assert(std::is_nothrow_move_constructible_v<JustB>);
    static_assert(!std::is_nothrow_move_constructible_v<AB>);

`AB`'s implicitly defaulted move constructor is potentially throwing, because
moving the `a` member might throw. `JustB`'s explicitly defaulted move constructor
is noexcept, because it merely needs to move the `B`.

## Conclusion: The vector pessimization

If you write a user-defined move constructor for your own type, no matter what
it does, if it has a body other than `=default`, you must mark it `noexcept`.

    struct Instrument {
        int n_;
        std::string s_;

        Instrument(const Instrument&) = default;

        // WRONG!!
        Instrument(Instrument&& rhs)
            : n_(std::exchange(rhs.n_, 0)),
              s_(std::move(s_))
            {}

        // RIGHT!!
        Instrument(Instrument&& rhs) noexcept
            : n_(std::exchange(rhs.n_, 0)),
              s_(std::move(s_))
            {}
    };

If you accidentally leave off the `noexcept`, then you'll get the _vector pessimization_:
every time you reallocate a `vector<Instrument>`, it will expensively make copies
instead of cheaply moving.

Worse, sometimes — especially on MSVC, using Microsoft's STL implementation —
you'll get the vector pessimization even on a Rule-of-Zero class. This is because
non-noexceptness propagates in the natural way. Having a single non-nothrow-movable
data member is all it takes to turn a Rule-of-Zero class non-nothrow-movable itself.

    struct Widget {
        std::list<int> data_;
        explicit Widget(int n) : data_(n) {}
    };

On MSVC, `Widget` will have a non-noexcept defaulted move constructor
and therefore get the vector pessimization: `vector<Widget>` will do pessimal,
copy-constructing reallocations. To fix this, you can (and arguably, should)
declare explicitly defaulted special members:

    struct Gadget {
        std::list<int> data_;
        explicit Gadget(int n) : data_(n) {}

        Gadget(Gadget&&) noexcept = default;
        Gadget(const Gadget&) = default;
        Gadget& operator=(Gadget&&) = default;
        Gadget& operator=(const Gadget&) = default;
    };

This tells the compiler, "I want `Gadget`'s move constructor to do the default
thing, but I also want it to be `noexcept`." So we avoid the vector pessimization,
as shown in [this Godbolt benchmark](https://godbolt.org/z/4GehWv58d).

Notice that _if_ the move constructor of `std::list` ever actually runs out of
memory and throws `std::bad_alloc` from inside `Gadget(Gadget&&)`, then that exception
_will_ slam into our artificial `noexcept` firewall and terminate your program.
That's _if_ you run out of memory. If this is a problem for your program, then
you might just have to take the performance hit of the vector pessimization.
For most programs, though, `std::bad_alloc` isn't a real concern — and the speed
of `vector::push_back` _might_ be!

This is why I recommend the following guidelines:

- If your type has a user-defined move constructor,
    you should _always_ declare that move constructor `noexcept`.

- If your Rule-of-Zero type has any non-nothrow-movable data members (especially common on MSVC),
    you should _consider_ giving it a defaulted move constructor explicitly marked `noexcept`.

- If nothing else, you should _be aware_ of the vector pessimization:
    know that `std::vector<T>` can turn moves into copies, during reallocation,
    if and only if `T(T&&)` hasn't been marked `noexcept`.

----

For another example of an implicitly generated special member doing
arguably the wrong thing, see:

* ["Self-assignment and the Rule of Zero"](/blog/2019/08/20/rule-of-zero-pitfall/) (2019-08-20)

For a fantastic amount of detail on the history of `noexcept`, written
literally as the feature evolved, see this post from way back in 2011:

* ["Using `noexcept`"](https://akrzemi1.wordpress.com/2011/06/10/using-noexcept/) (Andrzej Krzemieński, June 2011)
