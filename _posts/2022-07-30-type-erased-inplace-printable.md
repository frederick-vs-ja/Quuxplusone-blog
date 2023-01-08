---
layout: post
title: "Type-erased `InplaceUniquePrintable` benefits from `noexcept`"
date: 2022-07-30 00:01:00 +0000
tags:
  exception-handling
  llvm
  pearls
  relocatability
  type-erasure
excerpt: |
  Previously on this blog:
  ["Type-erased `UniquePrintable` and `PrintableRef`"](/blog/2020/11/24/type-erased-printable/) (2020-11-24).

  Now, here's a simple type-erased `InplaceUniquePrintable` (owning, value-semantic, move-only,
  non-heap-allocating). I don't claim that this one is bang-out-in-five-minutes-able,
  but on the other hand, it's the technique most commonly used in practice. The basic idea is to
  separate the _data_ of the erased type from its required _affordances_ or _behaviors_ —
  printing, relocating, and destroying. The data is stored as plain old bytes; the behaviors
  are stored as function pointers, which point to lambdas initialized in the constructor.
  I discuss this idea further in ["Back to Basics: Type Erasure"](https://www.youtube.com/watch?v=tbUCHifyT24) (CppCon 2019).
---

Previously on this blog:
["Type-erased `UniquePrintable` and `PrintableRef`"](/blog/2020/11/24/type-erased-printable/) (2020-11-24).

Now, here's a simple type-erased `InplaceUniquePrintable` (owning, value-semantic, move-only,
non-heap-allocating). I don't claim that this one is bang-out-in-five-minutes-able,
but on the other hand, it's the technique most commonly used in practice. The basic idea is to
separate the _data_ of the erased type from its required _affordances_ or _behaviors_ —
printing, relocating, and destroying. The data is stored as plain old bytes; the behaviors
are stored as function pointers, which point to lambdas initialized in the constructor.
I discuss this idea further in ["Back to Basics: Type Erasure"](https://www.youtube.com/watch?v=tbUCHifyT24) (CppCon 2019).

## `InplaceUniquePrintable`

[Godbolt.](https://godbolt.org/z/4zKjf5xGo) Notice that I completely arbitrarily
chose `sizeof(std::string)` for the size of the internal buffer.

    class InplaceUniquePrintable {
        alignas(std::string) unsigned char data_[sizeof(std::string)];
        void (*print_)(std::ostream&, const void *) = nullptr;
        void (*relocate_)(void *, const void *) noexcept = nullptr;
        void (*destroy_)(void *) noexcept = nullptr;
    public:
        template<class T, class StoredT = std::decay_t<T>>
        InplaceUniquePrintable(T&& t)
            // constraints are left as an exercise for the reader
        {
            static_assert(sizeof(StoredT) <= sizeof(std::string));
            static_assert(alignof(StoredT) <= alignof(std::string));
            static_assert(std::is_nothrow_move_constructible_v<StoredT>);
            ::new ((void*)data_) StoredT(std::forward<T>(t));
            print_ = [](std::ostream& os, const void *data) {
                os << *(const StoredT*)data;
            };
            relocate_ = [](void *dst, const void *src) noexcept {
                ::new (dst) StoredT(std::move(*(StoredT*)src));
                ((StoredT*)src)->~StoredT();
            };
            destroy_ = [](void *src) noexcept {
                ((StoredT*)src)->~StoredT();
            };
        }

        InplaceUniquePrintable(InplaceUniquePrintable&& rhs) noexcept {
            print_ = std::exchange(rhs.print_, nullptr);
            relocate_ = std::exchange(rhs.relocate_, nullptr);
            destroy_ = std::exchange(rhs.destroy_, nullptr);
            relocate_(data_, rhs.data_);
        }

        friend void swap(InplaceUniquePrintable& lhs, InplaceUniquePrintable& rhs) noexcept {
            std::swap(lhs.data_, rhs.data_);
            std::swap(lhs.print_, rhs.print_);
            std::swap(lhs.relocate_, rhs.relocate_);
            std::swap(lhs.destroy_, rhs.destroy_);
        }

        InplaceUniquePrintable& operator=(InplaceUniquePrintable&& rhs) noexcept {
            auto copy = std::move(rhs);
            swap(*this, copy);
            return *this;
        }

        ~InplaceUniquePrintable() {
            if (destroy_ != nullptr) {
                destroy_(data_);
            }
        }

        friend std::ostream& operator<<(std::ostream& os, const InplaceUniquePrintable& self) {
            self.print_(os, self.data_);
            return os;
        }
    };

### Example of use

    void printit(InplaceUniquePrintable p) {
        std::cout << "The printable thing was: " << p << "." << std::endl;
    }

    int main() {
        printit(42);
        printit("hello world");
    }

## Further improvements

Notice that we require our `StoredT` to fit in the `data_` buffer, and assert-fail
(at compile time) if that requirement is not met. However, if you're okay with heap-allocation,
it's easy to change those `static_assert`s into `if constexpr`s; then you can store
even a large printable object, simply by wrapping it in a heap-allocated pointer.
We just change `InplaceUniquePrintable`'s constructor to look like this, and
everything else can stay the same ([Godbolt](https://godbolt.org/z/K5fsre9ob)):

    constexpr bool fits_in_buffer =
        sizeof(StoredT) <= sizeof(std::string) &&
        alignof(StoredT) <= alignof(std::string) &&
        std::is_nothrow_move_constructible_v<StoredT>;

    if constexpr (fits_in_buffer) {
        ::new ((void*)data_) StoredT(std::forward<T>(t));
        print_ = ~~~ as before ~~~
    } else {
        using StoredPtr = StoredT*;
        auto p = std::make_unique<StoredT>(std::forward<T>(t));
        ::new ((void*)data_) StoredPtr(p.release());
        print_ = [](std::ostream& os, const void *data) {
            const StoredT *p = *(StoredPtr*)data;
            os << *p;
        };
        relocate_ = [](void *dst, const void *src) noexcept {
            std::memcpy(dst, src, sizeof(StoredPtr));
        };
        destroy_ = [](void *src) noexcept {
            StoredT *p = *(StoredPtr*)src;
            delete p;
        };
    }

Finally, we should avoid generating a non-trivial `destroy_` when `StoredT`
is trivially destructible; and we might consider doing the same for `relocate_`
when `StoredT` is known to be trivially relocatable.
([Godbolt](https://godbolt.org/z/3e691YMbc); [backup](/blog/code/2022-07-30-inplace-unique-printable.cpp).)

## Benefits from `noexcept`

Anyone who checks out this blog's [#exception-handling tag](/blog/tags/#exception-handling)
will know that I'm not big on `noexcept`. My rule of thumb is to put it on move constructors
(to undo the [vector pessimization](/blog/2022/08/26/vector-pessimization/)), optionally on `swap` and `operator=`, and (by default)
nowhere else. So you might be surprised to see that I've marked the `destroy_` and
`relocate_` function pointers as `noexcept`!

This is due to an awesome tip by Adrian Vogelsgesang, who
[points out](https://reviews.llvm.org/D130631#inline-1258790)
that adding `noexcept` on function pointers can sometimes allow them to be tail-called
from noexcept contexts, where an ordinary non-`noexcept` function pointer can't be.
Here's a simple example:

    struct Test {
        InplaceUniquePrintable p_;
        ~Test();
    };
    Test::~Test() = default;

`~Test()` here is going to call `~InplaceUniquePrintable()`, which is going to
call `*destroy_`. Recall that destructors (unlike other functions) are implicitly
`noexcept`, and recall that `noexcept` means "Insert a firewall here, that any
exceptions thrown from lower levels will smack into and terminate the program."
(That is, it's not a promise to the compiler that you _won't_ throw;
it's an instruction to insert extra code to _prevent_ throws from leaving this scope.)
When `~InplaceUniquePrintable()` calls `*destroy_`, the code generator must
decide whether it needs to insert one of those firewalls or not. If `destroy_`
is an ordinary function pointer, then it must. But if `destroy_` is marked
`noexcept`, then the compiler can assume any exceptions will have been caught
one level down, and no additional firewall is needed at this level — so this
function doesn't need a stack frame — so we can tail-call-optimize the call
to `*destroy_`!

The takeaway is that if you have a function pointer (or virtual function, I guess)
that you intend to call from a `noexcept` context such as a destructor or
move-constructor, then it is perhaps a performance benefit to mark that pointer's
data type with `noexcept`. Here's GCC's codegen for `Test::~Test()` with the code
as presented above ([Godbolt](https://godbolt.org/z/PeMM8Y3c8)):

    // void (*destroy_)(void *) noexcept = nullptr;

    _ZN4TestD2Ev:
      movq 48(%rdi), %rax
      testq %rax, %rax
      je .L1
      jmp *%rax  # TAIL CALL
    .L1:
      ret


And here it is without `noexcept`:

    // void (*destroy_)(void *) = nullptr;

    _ZN4TestD2Ev:
      movq 48(%rdi), %rax
      testq %rax, %rax
      je .L7
      subq $8, %rsp
      call *%rax  # NO TAIL CALL
      addq $8, %rsp
      ret
    .L7:
      ret

We get the same kind of benefit in `Test(Test&&)`; and notice
that I positioned the indirect call to `*relocate_` in tail position
for that reason. You and I know that `*relocate_` will never throw,
because we can see the code of the lambda assigned to it
a few dozen lines above; but because the call is indirect through a function pointer,
the compiler itself cannot see that `*relocate_` will never throw
(and therefore can be tail-called) unless we mark its type with
`noexcept`.

----

See also:

* ["What is Type Erasure?"](/blog/2019/03/18/what-is-type-erasure/) (2019-03-18)
* ["Back to Basics: Type Erasure"](https://www.youtube.com/watch?v=tbUCHifyT24) (CppCon 2019)
* ["The space of design choices for `std::function`"](/blog/2019/03/27/design-space-for-std-function/) (2019-03-27)
* ["It's not always obvious when tail-call optimization is allowed"](/blog/2021/01/09/tail-call-optimization/) (2021-01-09)
