---
layout: post
title: "A trivially copyable ticket for a `unique_ptr`"
date: 2019-09-21 00:02:00 +0000
tags:
  cppcon
  parameter-only-types
  relocatability
  smart-pointers
---

In [Chandler Carruth's CppCon 2019 talk "There Are No Zero-Cost Abstractions,"](https://www.youtube.com/watch?v=rHIkrotSwcc)
he talked a
lot (not exclusively, but a lot) about the hidden performance cost of `std::unique_ptr`.
See, `unique_ptr` may be the same _size_ as a native pointer, but because it has a
user-defined destructor, it is "non-trivial for purposes of ABI," and that means
it has to get passed on the stack.

For more on "trivial for purposes of ABI," see my previous blog post
["`[[trivial_abi]]` 101"](/blog/2018/05/02/trivial-abi-101/) (May 2018).

It occurred to me afterward that even if you didn't want to write your own custom `[[trivial_abi]]`-enabled
`MyUniquePtr` (maybe because you valued GCC compatibility — I think GCC still hasn't implemented
that attribute), you might be able to hack around it by simply creating a trivially copyable
"ticket for a `unique_ptr`," in the same sense that I describe `weak_ptr` as "a ticket for a
`shared_ptr`." The important thing about a ticket is that you can't use it directly; it doesn't
afford the user any operation except "redeem this ticket for a `unique_ptr`."

It would look something like [this (Godbolt)](https://godbolt.org/z/FiKs8w):

    template<class T>
    struct ticket {
        ticket() = default;
        explicit ticket(T *p) : p_(p) {}
        explicit ticket(std::unique_ptr<int>&& p) : p_(p.release()) {}
        std::unique_ptr<T> redeem() && { return std::unique_ptr<T>(std::exchange(p_, nullptr)); }
    private:
        T *p_ = nullptr;
    };

And then Chandler's test harness, which originally looked something like this
and produced 27 lines of assembly —

    void bar(int*);
    void baz(std::unique_ptr<int>);

    void foo(std::unique_ptr<int> p) {
        bar(p.get());
        baz(std::move(p));
    }

— would instead look something like this and produce 19 lines of assembly —

    void bar(int*);
    void baz(ticket<int>);

    void foo(ticket<int> t) {
        std::unique_ptr<int> p = std::move(t).redeem();
        bar(p.get());
        baz(ticket<int>(std::move(p)));
    }

So what's the catch? Well, it's a big one. The trivially copyable `ticket` object
by definition has a trivial destructor. So it doesn't consider itself to "own" the
heap-allocated object. If an exception is thrown during the time the heap allocation
is managed only by the ticket, then the allocation will be leaked!

    void use(ticket<int> t, int u);
    int thrower() { throw "oops"; }

    void test() {
        auto p = std::make_unique<int>(42);
        use(
            ticket<int>(std::move(p)), // lose ownership...
            thrower()  // ...and leak the allocation!
        );
    }

However, as Chandler himself pointed out, this is only a problem if your codebase
uses exceptions at all! If you don't use exceptions, then you don't have this issue,
and _maybe_ the idea of a "trivially copyable ticket for a `unique_ptr`" might be
interesting to you.

What would make this pattern actually usable, I think, would be if the language had some
way to say "ABI-wise, I take a parameter of type `X`; but the first and only thing
I'm ever going to do with that parameter is to convert it to type `Y`." Something like
this fantasy syntax:

    void baz(ticket_view<int>);

    void foo(ticket_view<int> -> std::unique_ptr<int> p) {
        bar(p.get());
        baz(std::move(p));
    }

(Here I've renamed `ticket` to `ticket_view`, and given it an implicit constructor
from `unique_ptr`, and given it an explicit conversion to `unique_ptr` instead of
a named method `redeem()`. This emphasizes its similarity to `string_view` as a
[parameter-only type](/blog/tags/#parameter-only-types).)
