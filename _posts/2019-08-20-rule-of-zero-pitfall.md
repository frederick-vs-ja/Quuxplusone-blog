---
layout: post
title: "Self-assignment and the Rule of Zero"
date: 2019-08-20 00:01:00 +0000
tags:
  move-semantics
  pitfalls
  smart-pointers
---

The other day [on Slack](https://cpplang.slack.com/archives/C21PKDHSL/p1565799396205900),
someone posted a minimal `unique_ptr` and asked for code review.
Its move-assignment operator looked something like this:

    template<class T>
    struct UniquePtr {
        T *ptr_;

        UniquePtr& operator=(UniquePtr&& rhs) noexcept {
            delete ptr_;
            ptr_ = std::exchange(rhs.ptr_, nullptr);
            return *this;
        }
    };

Daniel Frey pointed out that this assignment operator is not safe with respect to
self-move-assignment.

    UniquePtr<int> p = MakeUnique<int>(42);
    p = std::move(p);

We `delete` the controlled pointer, and then replace it with `nullptr`, and then replace
it with the result of `std::exchange` (which is the original pointer). Result: `p` now
controls a pointer to deleted memory.

----

Furthermore, Daniel pointed out, that assignment operator is not safe even with respect to
"assignment from a sub-part of self's controlled object"!

    struct Widget {
        UniquePtr<Widget> m;
    };

    UniquePtr<Widget> p = MakeUnique<Widget>();  // A
    p->m = MakeUnique<Widget>();  // B
    p = std::move(p->m);  // C

On line C, we take an rvalue reference to `p->m`, and then call `UniquePtr::operator=`.
We `delete p.ptr_`, which destroys the `Widget` controlled by `p.ptr_`. Our rvalue reference
to `p->m` is now a dangling reference. Now we exchange `nullptr` with garbage loaded from
that dangling reference (corrupting our heap with that write!), and assign the garbage to `p.ptr_`.

----

Furthermore, we can run into this pitfall with _any_ assignment operator which is not implemented
via copy-and-swap. The following Rule-of-Zero code ([Godbolt](https://godbolt.org/z/fKx4KjrPf))
has undefined behavior:

    struct B {
        std::unique_ptr<B> m = nullptr;
        int i = 42;
    };
    int main() {
        B b;
        b.m = std::make_unique<B>();  // D
        b = std::move(*b.m);  // E
        return b.i;
    }

On line E, we call the implicitly defaulted `B::operator=(B&&)`. Being defaulted, it performs
memberwise move-assignment ([[class.copy.assign]/12](http://eel.is/c++draft/class.copy.assign#12)).
The first member to be move-assigned is `m`. Move-assigning from `(*b.m).m` into `b.m` causes
the deletion of the heap object allocated on line D. Our rvalue reference to `(*b.m)` is now a
dangling reference. Now the defaulted move-assignment operator move-assigns member `i` — from a
dangling reference! So `b.i` gets overwritten with garbage loaded from a deallocated part of the
heap. In the Godbolt example, I simulate a common malloc behavior and overwrite deallocated
blocks with `0xAA`, which means that `b.i` receives `0xAAAAAAAA` (and so `main` returns
`0xAA` or 170) instead of the expected `42`.

This isn't even a problem unique to move-assignment. The same thing would happen with
copy-assignment of a `B` with a `shared_ptr` member ([Godbolt](https://godbolt.org/z/fY9b71Thb)).
The problem is not fixable by library vendors — there's no flaw in the implementation
of the smart pointers' assignment operators. The problem is with the _defaulted_ implementation
of `B`'s assignment operator!

----

Does this mean you should stop using the Rule of Zero entirely? Definitely not! The Rule of Zero
is great.

But if you're in a domain where you expect your client frequently to assign between different
parts of a nested data structure — Daniel gives [taocpp/json](https://github.com/taocpp/json)
as an example — then you ought to be very cautious any time you're relying on the compiler to
generate `operator=` for you. (Or any other function that frees resources.)

The ultimate answer here, IMO, is that the compiler ought to have some way to auto-generate `swap`
functions for us, and then all defaulted `operator=`s should be re-specified to generate the copy-and-swap
idiom instead of memberwise assignment. (Of course the as-if rule would still apply.)
I don't think there's any chance of getting that in Standard C++, but it would be a pretty
neat experiment in non-standardness.
