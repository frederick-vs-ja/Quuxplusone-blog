---
layout: post
title: 'View types with metadata cause problems'
date: 2018-05-30 00:02:00 +0000
tags:
  library-design
  parameter-only-types
  pitfalls
  sg14
  wg21
---

Vinnie Falco, in a draft of an upcoming paper P1100, [writes](https://rawgit.com/vinniefalco/cpp/master/d1100r0.html):

> A dynamic string buffer contains a reference to the underlying string.
> Copies of a dynamic string buffer refer to the same string. Note that the
> dynamic string buffer also contains some state: the `size_` and `max_size_`
> data members. This additional metadata informs the dynamic string buffer
> of the boundaries between the readable and writable bytes, as well as
> the maximum allowed size of the total of the readable and writable bytes.

Vinnie quotes [LWG 3072, filed by Chris Kohlhoff](http://cplusplus.github.io/LWG/lwg-active.html#3072):

> Asio ... performs `DECAY_COPY(b)`
> in the `async_read`, `async_write`, and `async_read_until` initiating functions.
> All operations performed on `b` are actually performed on that decay-copy,
> or on a move-constructed descendant of it. _The copy is intended to refer to
> the same underlying storage and be otherwise interchangeable with the original
> in every way._ [Ed: emphasis added]

The core of the problem is:

> When only one composed operation handles the dynamic buffer, things seem to work.
> However, if a composed operation wishes to invoke _another_ composed operation on that
> dynamic buffer, [...] it cannot do so without depending on undefined behavior.

We saw the same problem come up (at least teachability-wise) with
[P0059R4 `ring_span`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0059r4.pdf),
on which I am a coauthor.
A `ring_span` contains a reference to the underlying buffer, and also contains
some state: the `m_size` and `m_front_idx` data members. This additional metadata
informs the `ring_span` object of the boundaries between the elements which are
currently "inside the ring-buffer" and those which are currently "unused capacity."

Like Vinnie, I found that this was a confusing design for a C++ class type.
A `ring_span` _looks_ like a non-owning view type, so the temptation is very
strong to write e.g.

    void consume_from(ring_span<Task> r) {
        Task t = r.pop_front();
        use(t);
    }

    void produce_to(ring_span<Task> r) {
        Task t = newtask();
        r.push_back(std::move(t));
    }

passing the `ring_span` object by value the same way you'd pass a [`string_view`](https://abseil.io/tips/1)
or a [`function_ref`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0792r2.html).
However, if you do this, you make a _copy_ of the `size_` and `front_idx_` metadata members,
and all your side-effects act on that _copy_, and the caller never sees them!
Even if your program doesn't crash as a result, you've still got two `ring_span`
objects who both think they know what's going on with the buffer, and at least one of them
is wrong.

## "Data structure views"

The other day I had a discussion with Charley Bay, Niall Douglas, and Bob Steagall
on the subject of what at-least-some-of-them wanted to call "addressing engine" types.
The idea, essentially, is that we can think of a `ring` as simply a `ring_span` slapped
on top of a `std::vector` (or indeed a `std::array`) — and how can we generalize this idea?

The Standard already provides three types kinda-sorta of this nature: `stack`, `queue`, and
`priority_queue`. There are two big differences between `std::priority_queue` and P0059 `ring_span`:

- `priority_queue` owns its elements. The nature of its owning container is template-parameterized,
  but it definitely _is_ owning.

- Imagine for a minute that `priority_queue` was not owning. Then, still, `priority_queue` holds
  _no metadata_ — if you know the number and arrangement of the elements in its storage container,
  you have everything you need to reconstruct a new `priority_queue` which would be an exact copy
  of the original. Contrariwise, it is impossible to reconstruct a `ring_span` given only the
  arrangement of its storage container; you also need to know which element to consider the "front"
  of the ring-buffer.

Come to think of it, this boils down to just _one_ difference: `priority_queue` is backed by a
_resizable, owning STL container_, whereas `ring_span` is backed by a _raw range_.
If we designed a `priority_queue` backed by a raw range, then either it would not be resizable
(which is actually a [perfectly cromulent design for a priority queue!](/blog/2018/04/27/pq-replace-top/#in-other-words-the-operation-tha))
or else it would have to keep track of its "current size" (`size_`) as a data member — at
which point, passing it around by value would be just as dangerous as passing around a
`ring_span` or a `dynamic_string_buffer` by value.

Notice that when you pass a `string_view` around by value, you don't have these problems, even though
`string_view` _also_ contains a "size" data member and has operations such as
[`sv.remove_prefix(k)`](https://en.cppreference.com/w/cpp/string/basic_string_view/remove_prefix)
which change the "size." The reason, I think, is that when you change the size of a `string_view`,
you intuitively understand that you are not _destroying_ the unviewed elements. Whereas, when you
"pop" an element off of a `priority_queue`, you understand that the popped element is really _gone_,
not merely _unviewed_.

P0059's `ring_span` tries to split the difference. Its mental model is that unviewed elements are
still there, just like `string_view`... _but_, whenever an element transitions from the "viewed" state
to the "unviewed" state, we run a special bit of code (customizable via the template parameters)
called the "popper," which defaults to leaving the popped element in a moved-from state.

I have come to the position that the only really foolproof way to use P0059 `ring_span` is to wrap it
up in a container adaptor, something like this:

    template<class T, class Container = std::vector<T>>
    class ring {
        Container c;
        ring_span<T> rs;
    public:
        explicit ring(size_t capacity) :
            c(capacity), rs(c.begin(), c.end()) {}
        void push_back(T t) { rs.push_back(t); }
        T pop_front() { return rs.pop_front(); }
        size_t size() const { return rs.size(); }
    };

This doesn't mean that `ring_span` is useless on its own; actually I think it might be quite useful in
some (experts-only) cases.

Which raises the question: Does it make sense to have `foo_span` types for other containers too?
Could we imagine a `vector_span` that doesn't own its memory, doesn't know anything about allocators,
but does track its own `size_` distinct from its maximum capacity?

I believe that we _could_ imagine such a `vector_span`, but it would have all the problems that `ring_span`
has: it tempts you to inappropriately pass-by-value, and it raises troubling questions about the lifetimes
of "unviewed" objects (which even `ring_span`'s "popper" solution doesn't fix as much as I'd like to).
And unlike `ring_span`, it seems not to abstract away anything useful from the owning version of the
container. Consider:

    template<class T, class Container = std::vector<T>>
    class vector {
        Container c;
        vector_span<T> vs;
    public:
        explicit vector(size_t capacity) :
            c(capacity), vs(c.begin(), c.end()) {}
        void push_back(T t) { vs.push_back(t); }
        T pop_front() { return vs.pop_front(); }
        size_t size() const { return vs.size(); }
    };

Here we have combined a `vector_span` with an owning STL container to produce an owning `vector`.
Unfortunately, the only sensible choice for the underlying STL container is... `vector` itself,
leading to a silly circular definition. Why does this example seem silly? Is `vector` just _too simple_
to be a suitable testbed for our "non-owning non-allocating view" idea?

Taking it even further, could we _imagine_ a non-owning, non-allocating type analogous to `std::set`
(that is, a red-black tree) or `std::list`?  I have tried, but I can't imagine what such a thing
would look like.

So we seem to have a continuum here, with `vector` at the near end, `ring` in the middle, and `set` at
the far end. At the near end, the idea of a "non-owning, non-allocating view" is quickly reduced
_ad absurdum_; at the far end, the idea is unimaginable (at least to me); and there's a sweet spot
in the middle, exemplified by `ring_span` and `dynamic_string_buffer`, where the idea seems workable
but has significant teachability and usability problems.

Disclaimer: I may be too close to `ring_span`, and too relatively far from `vector_span` and
`set_span`, to really judge how "workable" any of them are. Maybe you can implement a `set_span`
and show me what it looks like; or maybe you consider `ring_span`'s flaws to be fatal (meaning
that the spot in the middle is no sweeter than anywhere else on the continuum).

Anyway, the take-home point here is: when you design a type that represents a _non-owning view
of some objects, plus some other data members_, you are signing up for a whole lot of trouble.
I recommend against it.
