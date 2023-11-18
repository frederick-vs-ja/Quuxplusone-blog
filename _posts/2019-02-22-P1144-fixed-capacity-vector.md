---
layout: post
title: "P1144 case study: Moving a `fixed_capacity_vector`"
date: 2019-02-22 00:01:00 +0000
tags:
  kona-2019
  relocatability
  sg14
---

I wrote up this example on Wednesday morning for my presentation of
[P1144 "Object relocation in terms of move plus destroy"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1144r2.html)
at the Kona committee meeting.

[Here's the complete code on Godbolt](https://p1144.godbolt.org/z/PjWgIM)
([backup](/blog/code/2019-02-22-fixed-capacity-vector.cpp)).
Consider a skeleton `fixed_capacity_vector`:

    template<class T, int Cap>
    class fixed_capacity_vector {
        int size_ = 0;
        alignas(T) char buffer_[Cap][sizeof(T)];
    public:
        fixed_capacity_vector(fixed_capacity_vector&& rhs) noexcept {
            // YOUR CODE GOES HERE
        }
        ~fixed_capacity_vector() {
            for (int i=0; i < size_; ++i) {
                (*this)[i].~T();
            }
        }
        template<class... Args>
        void emplace_back(Args&&... args) {
            // assert(size_ < Cap);
            new ((void*)buffer_[size_]) T(std::forward<Args>(args)...);
            size_ += 1;
        }
        T *begin() { return (T*)buffer_[0]; }
        T *end() { return (T*)buffer_[size_]; }
    };

Consider a test harness using this `fixed_capacity_vector`.

    fixed_capacity_vector<TYPE, 100>
    test(fixed_capacity_vector<TYPE, 100> vec) {
        vec.emplace_back(nullptr);
        return vec;
    }

Let type `T` be whatever you like — `int*`, `unique_ptr<int>`, `shared_ptr<int>`.
What's the fastest way to move-construct a `fixed_capacity_vector` when it's
returned from a function, as in this example?

Boost's `static_vector` does this:

    fixed_capacity_vector(fixed_capacity_vector&& rhs) noexcept {
        size_ = rhs.size_;
        std::uninitialized_move(rhs.begin(), rhs.end(), this->begin());
    }

I suggest that a faster way would be to do this instead:

    fixed_capacity_vector(fixed_capacity_vector&& rhs) noexcept {
        size_ = rhs.size_;
        std::uninitialized_relocate(rhs.begin(), rhs.end(), this->begin());
        rhs.size_ = 0;
    }

With my P1144-enabled compiler and library, I compiled my test case nine different
ways. The compiler command line looks like this:

    clang++ -S test.cpp -std=c++17 -O3 -DTYPE='int*'
    wc -l test.s
    grep memcpy test.s

To test the "move" strategy, I add `-DUSE_RELOCATE_TO_MOVE=0` to the command line.
To test the "relocate (no P1144)" strategy, I add `-DUSE_RELOCATE_TO_MOVE=1 -D_LIBCPP_TRIVIALLY_RELOCATABLE=`.
To test the "relocate (with P1144)" strategy, I add `-DUSE_RELOCATE_TO_MOVE=1`.

The results:

| `-DTYPE=` | Strategy | Code size | Uses memcpy? |
|:---------:|:---------:|:---------:|:---------:|
| `int*` | move                     | 116 | no |
| `int*` | relocate (no P1144)      | 42 | yes |
| `int*` | relocate (with P1144)    | 42 | yes |
| `unique_ptr<int>` | move                  | 95 | no |
| `unique_ptr<int>` | relocate (no P1144)   | 96 | no |
| `unique_ptr<int>` | relocate (with P1144) | 42 | yes |
| `shared_ptr<int>` | move                  | 98 | no |
| `shared_ptr<int>` | relocate (no P1144)   | 99 | no |
| `shared_ptr<int>` | relocate (with P1144) | 45 | yes |

----

See also:

* ["`Quuxplusone/SG14` now has `inplace_vector`"](/blog/2023/10/20/sg14-inplace-vector/) (2023-10-20)
