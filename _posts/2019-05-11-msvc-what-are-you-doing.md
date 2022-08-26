---
layout: post
title: "MSVC can't handle move-only exception types"
date: 2019-05-11 00:01:00 +0000
tags:
  cppnow
  exception-handling
  move-semantics
  pitfalls
  relocatability
excerpt: |
  Here's my second post from C++Now 2019!

  [On Friday morning](https://www.youtube.com/watch?v=kO0KVB-XIeE),
  Andreas Weis gave a very good summary of how exception handling works in C++. I particularly
  liked that he didn't just focus on the Itanium ABI (as I would certainly have been tempted to
  do), but showed in-depth knowledge of how exception handling works on MSVC and also on
  embedded platforms. The only thing that could have made Andreas's talk better would have
  been if he'd mentioned my ["`dynamic_cast` From Scratch"](https://www.youtube.com/watch?v=QzJL-8WbpuU)
  (CppCon 2017).
---

Here's my second post from C++Now 2019!

[On Friday morning](https://www.youtube.com/watch?v=kO0KVB-XIeE),
Andreas Weis gave a very good summary of how exception handling works in C++. I particularly
liked that he didn't just focus on the Itanium ABI (as I would certainly have been tempted to
do), but showed in-depth knowledge of how exception handling works on MSVC and also on
embedded platforms. The only thing that could have made Andreas's talk better would have
been if he'd mentioned my ["`dynamic_cast` From Scratch"](https://www.youtube.com/watch?v=QzJL-8WbpuU)
(CppCon 2017). :)

Andreas discussed the different ways an implementation could allocate memory for a thrown exception.
The familiar way (if you're an Itanium ABI chauvinist like me) is that the code calls
`__cxa_allocate_exception`, which is basically `malloc`, so all your in-flight exception objects
go on the heap all the time. Andreas mentioned the "emergency buffer" set aside so that even when
`malloc` is exhausted, you'll be able to throw `bad_alloc`. But he also mentioned that on MSVC,
sometimes (always?), the exception object is initially allocated on the stack!

That is, MSVC will allocate the in-flight exception object into the thrower's own stack frame.
Then, when the appropriate catch block is found, the catch handler will execute with its stack pointer
pointing even _further_ below the start of the thrower's stack frame (almost as if the site of the `throw`
were instead a call to a made-up function consisting of the catch handler code). When the catch-handler
"returns," the stack pointer flies all the way back up to the catching function's original level.

> Destructors for stack variables in between the thrower and the catcher are still correctly
> destroyed, prior to executing the catch handler.

But if you allocate the exception object on the stack, how do you implement
[`std::exception_ptr`](https://en.cppreference.com/w/cpp/error/exception_ptr), which is supposed
to extend the lifetime of a (heap-allocated) exception object and allow it to be passed even between
threads? (`exception_ptr` is basically a `shared_ptr` with a very curtailed public interface.)
You can't have an `exception_ptr` pointing into the stack — as soon as you left the last
`catch` handle and the stack pointer flew back up, you'd quickly trash the stack slot you were
using to store the exception object.

So what MSVC does is, when you construct an `exception_ptr`, it _copies_ the exception object
from the stack to the heap. From that point on, it can proceed similarly to the Itanium ABI with
its heap-allocated exception objects.
You can observe this happening [on rextester](https://rextester.com/BFXO54131):

    #include <exception>
    #include <stdio.h>

    struct Widget {
        Widget() { puts("constructing the exception object of type Widget on the stack"); }
        Widget(const Widget&) { puts("copying the Widget object from the stack to the heap"); }
        ~Widget() { puts("destroying a Widget object"); }
    };

    std::exception_ptr ex;

    void bar() {
        puts("Now throwing Widget...");
        throw Widget();
    }

    void foo() {
        try {
            bar();
        } catch (...) {
            puts("Found the catch handler.");
            puts("Now calling current_exception...");
            ex = std::current_exception();
        }
    }

    int main() {
        foo();
        puts("Now setting ex to nullptr...");
        ex = nullptr;
        puts("Now exiting main()");
    }

This program prints

    Now throwing Widget...
    constructing the exception object of type Widget on the stack
    Found the catch handler.
    Now calling current_exception...
    copying the Widget object from the stack to the heap
    destroying a Widget object
    Now setting ex to nullptr...
    destroying a Widget object
    Now exiting main()

Compare to the Itanium ABI (Linux or OSX), where it prints

    Now throwing Widget...
    constructing the exception object of type Widget on the stack
    Found the catch handler.
    Now calling current_exception...
    Now setting ex to nullptr...
    destroying a Widget object
    Now exiting main()

On the Itanium ABI, my output messages are a lie: we _don't_ construct the exception object
on the stack, but rather store it on the heap from the get-go. Only MSVC does this weird
"construct it on the stack and then move it to the heap" business.

Finally, when you call `std::rethrow_exception(ex)` on MSVC, it seems to copy the exception object
_back_, from the heap to the stack — which strikes me as surprisingly wasteful, but might be
necessary for backward compatibility with catch handlers who expect the exception object to
be stack-allocated.

----

Notice that even though we wrote `catch (...)`, MSVC still knows how to call the copy constructor
of `Widget` and how to call the destructor of `Widget`. Those two pieces of information are held
in MSVC's exception-handling data structures (essentially a form of
[type erasure](/blog/2019/03/18/what-is-type-erasure/)).

However, what if we were to throw an exception object that was not copyable at all?
Well, technically, that would be ill-formed. [[except.throw/5](http://eel.is/c++draft/except.throw#5)]:

> When the thrown object is a class object, the constructor selected for the
> copy-initialization *as well as the constructor selected for a copy-initialization
> considering the thrown object as an lvalue* shall be non-deleted and accessible,
> even if the copy/move operation is elided.

In other words, it is _ill-formed_ to throw a move-only type such as `unique_ptr`!
[No compiler vendor implements this rule.](https://godbolt.org/z/nKSBf7) For the non-MSVC ones
who allocate exception objects on the heap and never copy them, the rule serves no purpose.
For MSVC... well, let's see. [(Rextester.)](https://rextester.com/RROWY60717)

    struct Copyable {
        Copyable() { puts(" creating"); }
        Copyable(const Copyable&) { puts(" copying"); }
        ~Copyable() { puts(" destroying"); }
    };

    std::exception_ptr ex;

    int main()
    {
        try {
            puts("Throwing an exception of type Copyable...");
            throw Copyable();
        } catch (...) {
            puts("Found the catch block.");
            puts("Now calling current_exception...");
            ex = std::current_exception();
            try {
                puts("Now calling rethrow_exception...");
                std::rethrow_exception(ex);
            } catch (...) {
                puts("Found the next catch block.");
            }
        }

        puts("Now setting ex to nullptr...");
        ex = nullptr;
        puts("Now exiting main()");
    }

This program prints

    Throwing an exception of type Copyable...
     creating
    Found the catch block.
    Now calling current_exception...
     copying
    Now calling rethrow_exception...
     copying
    Found the next catch block.
     destroying
     destroying
    Now setting ex to nullptr...
     destroying
    Now exiting main()

But replace `Copyable` with `MoveOnly`:

    struct MoveOnly {
        MoveOnly() { puts(" creating"); }
        MoveOnly(MoveOnly&&) { puts(" moving"); }
        ~MoveOnly() { puts(" destroying"); }
    };

Now it prints

    Throwing an exception of type MoveOnly...
     creating
    Found the catch block.
    Now calling current_exception...
    Now calling rethrow_exception...
    Found the next catch block.
     destroying
     destroying
    Now setting ex to nullptr...
     destroying
    Now exiting main()

That is, MSVC's runtime seems to assume that when an exception type has no copy constructor
(or a deleted copy constructor), then it should use `memcpy` to "copy" the exception object from
the stack to the heap and back!

Yet, also notice, the runtime still believes it knows how to _destroy_ objects of type `MoveOnly`.
Therefore we get three calls to the destructor (destroying the original stack object, the heap-allocated
copy, and the rethrown stack copy), matching up to only one call of the constructor.

# Conclusion

If you `throw std::make_unique<int>();` on MSVC, and then traffic it through an `exception_ptr`,
you'll get a double- or triple-delete of the controlled pointer. And since `exception_ptr` is used
internally by STL facilities such as `std::future` and `std::async`, you really can't be sure when
you're using `exception_ptr` and when you're not.

[Example:](https://rextester.com/KYGNQ69337)

    class danger : public std::runtime_error {
        using runtime_error::runtime_error;
        std::unique_ptr<int> state_ = std::make_unique<int>();
    };

    int main() {
        auto f = std::async([]() {
            throw danger("hello");
        });
        try {
            f.get();
        } catch (...) {
            std::cout << "Double-delete incoming!" << std::endl;
        }
    }

The above program is ill-formed (by [except.throw]/5). But every vendor will compile it successfully.
And every vendor will run it successfully — _except for MSVC._ MSVC will triple-delete the `state_`
member of the exception object (once from the stack, and then again from the heap, and then
again from the stack), leading to heap corruption and crash.

----

When I heard about this in Andreas's talk, initially I thought, "Hey, here's another application
for trivial relocatability!" MSVC is basically assuming it's okay to `memcpy` these objects around,
which sounds related to [P1144](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1144r3.html).
But notice that MSVC is merely replacing the _move constructor_ with
`memcpy` — they're still explicitly calling the destructor on the moved-from object.
So this is _not_ an application of the P1144 "trivially relocatable" concept.
Instead, the practical requirement on exception types in MSVC is

    template<class Ex>
    struct is_exception_type : std::bool_constant<
        std::is_copy_constructible_v<Ex> ||
        (std::is_trivially_move_constructible_v<Ex> && std::is_trivially_destructible_v<Ex>)
    > {};
