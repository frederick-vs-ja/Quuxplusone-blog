---
layout: post
title: "Update on my trivial `swap` prize offer"
date: 2023-03-04 00:01:00 +0000
tags:
  llvm
  relocatability
excerpt: |
  In ["Trivial relocation, `std::swap`, and a $2000 prize"](/blog/2023/02/24/trivial-swap-x-prize/) (2023-02-24), I wrote:

  > I hereby offer a $2000 (USD) cash prize to the first person who produces an efficient and conforming implementation
  > of `std::swap_ranges`, `std::rotate`, and `std::partition` — using `memmove`-based trivial swapping where possible,
  > but falling back to a conforming `std::swap` implementation when the objects involved might be
  > potentially overlapping subobjects.
  >
  > [Here](https://godbolt.org/z/aa3aeqd3q) ([backup](/blog/code/2023-02-24-prize-test-suite.cpp))
  > is the test suite that the winning implementation will pass. [...]

  This generated a few helpful comments on [/r/cpp](https://old.reddit.com/r/cpp/comments/11aserj/trivial_relocation_stdswap_and_a_2000_prize/),
  zero actual submissions, and enough motivation (coupled with those helpful /r/cpp comments) that I went off
  and implemented a solution myself. So as of 2023-03-03, [my libc++ fork passes](https://godbolt.org/z/8x89ahxcP)
  all eight of my test cases, and I hereby award myself the prize for libc++! ;)

  In the spirit of fair play, and also because I really want to see P1144 implementations in other STLs,
  I'm keeping the prize open for patches against libstdc++ and Microsoft STL. If you can make libstdc++
  or Microsoft STL pass [the test suite](/blog/code/2023-02-24-prize-test-suite.cpp), please tell me about it, and I'll happily fork over.
  Offer ends December 31st, 2024, just so I don't have to worry about forgetting to formally close the window
  and having someone try to claim the prize ten years from now.
---

In ["Trivial relocation, `std::swap`, and a $2000 prize"](/blog/2023/02/24/trivial-swap-x-prize/) (2023-02-24), I wrote:

> I hereby offer a $2000 (USD) cash prize to the first person who produces an efficient and conforming implementation
> of `std::swap_ranges`, `std::rotate`, and `std::partition` — using `memmove`-based trivial swapping where possible,
> but falling back to a conforming `std::swap` implementation when the objects involved might be
> potentially overlapping subobjects.
>
> [Here](https://godbolt.org/z/aa3aeqd3q) ([backup](/blog/code/2023-02-24-prize-test-suite.cpp))
> is the test suite that the winning implementation will pass. [...]
> As of 2023-02-22, [Clang trunk](https://godbolt.org/z/K8hqqjvhs) passes only tests 6 and 7;
> my P1144 fork of libc++ passes only tests 1, 3, 5, and 8.
>
> [...] By the way, if you patch GCC to natively recognize trivially relocatable types,
> I'd be thrilled to assist you in getting your patched version its own place in the dropdown on
> Godbolt Compiler Explorer!

This generated a few helpful comments on [/r/cpp](https://old.reddit.com/r/cpp/comments/11aserj/trivial_relocation_stdswap_and_a_2000_prize/),
zero actual submissions, and enough motivation (coupled with those helpful /r/cpp comments) that I went off
and implemented a solution myself. So as of 2023-03-03, [my libc++ fork passes](https://godbolt.org/z/8x89ahxcP)
all eight of my test cases, and I hereby award myself the prize for libc++! ;)

In the spirit of fair play, and also because I really want to see P1144 implementations in other STLs,
I'm keeping the prize open for patches against libstdc++ and Microsoft STL. If you can make libstdc++
or Microsoft STL pass [the test suite](/blog/code/2023-02-24-prize-test-suite.cpp), please tell me about it, and I'll happily fork over.
Offer ends December 31st, 2024, just so I don't have to worry about forgetting to formally close the window
and having someone try to claim the prize ten years from now.

Alternatively, if you find a bug in my implementation — if you concoct a new test case that it still
handles incorrectly — please tell me about it, and I'll un-award myself the prize. :)


## The winning(?) strategy explained

The following code is from libc++, which means it'll look weird.
I've removed some of the library-specific macros, but I've left in other library quirks,
such as `_Uglified __names` and the use of `std::addressof` to prevent ADL on `operator&`.
One of these days I'll do a blog post on STL-implementor style.

<b>Step 1.</b> `std::swap` can't assume it's swapping complete objects, so it must never swap the
trailing padding bytes. We need to implement `datasizeof` in the library. Since we're the implementation,
we don't need to work portably — in fact I only care about Clang — so I just did this:

    template <class _Tp>
    struct __libcpp_datasizeof {
        struct _Up {
            [[no_unique_address]] _Tp __t_;
            char __c_;
        };
        static const size_t value = offsetof(_Up, __c_);
    };

<b>Caveat 1.</b> Thankfully, Clang supports `[[no_unique_address]]` in all dialects C++11 and later.
But it doesn't support it in C++03 mode; not even as `__attribute__((no_unique_address))`. I don't know
why. Also, Clang fails to support an uglified spelling `__no_unique_address`, so it
encroaches on the user's namespace in C++11 through C++17. Effectively, Clang retroactively
makes `no_unique_address` into a reserved word in pre-C++20 dialects. That's
[not conforming](https://github.com/llvm/llvm-project/issues/61196), and I'd love to see Clang fix it.
But none of this matters for my purposes.
[EDIT 2023-03-07: Never mind, Clang _does_ support `__no_unique_address__` when you affix leading _and_
trailing underscores. It still lacks support in C++03 mode. As of this writing, libc++ trunk fails to
use the uglified `__no_unique_address__`, but they'll likely fix that soon.]

<b>Step 2.</b> Inside `std::swap`, use `__libcpp_memswap` to swap `__libcpp_datasizeof` bytes.
Our entry point here is actually called `__generic_swap`, because it's called by both `std::swap`
and (conditionally) `std::ranges::swap`.

    template <class _Tp>
    void __generic_swap(_Tp& __x, _Tp& __y)
      noexcept(is_nothrow_move_constructible<_Tp>::value &&
               is_nothrow_move_assignable<_Tp>::value)
    {
      if (__libcpp_is_trivially_relocatable<_Tp>::value &&
          !is_volatile<_Tp>::value &&
          is_empty<_Tp>::value) {
        // Swap nothing.
    #ifndef _LIBCPP_CXX03_LANG
      } else if (__libcpp_is_trivially_relocatable<_Tp>::value &&
                 !is_volatile<_Tp>::value &&
                 !__libcpp_is_constant_evaluated()) {
        // Swap only "datasizeof" bytes.
        std::__libcpp_memswap(std::addressof(__x), std::addressof(__y),
                              __libcpp_datasizeof<_Tp>::value);
    #endif
      } else {
        _Tp __t(std::move(__x));
        __x = std::move(__y);
        __y = std::move(__t);
      }
    }

<b>Caveat 2.</b> Notice that we can't use the `__libcpp_memswap` codepath at constexpr time (because
its pointer math relies on casting `void*` to `char*`). That's no problem, though;
we just use [`std::is_constant_evaluated`](https://en.cppreference.com/w/cpp/types/is_constant_evaluated)
to take the unoptimized path at constexpr time.

<b>Step 3.</b> This is the really big one. Use `__libcpp_memswap` inside `std::swap_ranges`!
libc++ recently refactored the way its `std::copy` and `std::copy_backward` use `memcpy`
(thanks, Konstantin Varlamov!), and now it doesn't take much to reuse the same machinery
in `swap_ranges`.

No code snippet for this step, because the machinery is so hairy and spread across several files.

The important thing here is that `std::swap_ranges` should _not_ just do `std::swap` in a loop,
because `swap` now might swap less than the full object's footprint. The optimized codepath in
`swap_ranges` jumps straight to `__libcpp_memswap` on the entire range.

----

...And then we're done! libc++'s `std::rotate` already works in terms of `std::swap_ranges`,
so it picks up the optimized code for free — the exact thing I was worried wouldn't happen
when the optimization was confined to `std::swap`. Adding the optimized codepath to `swap_ranges`
turned out to be the secret ingredient.

It's hard to tell whether the change really produces faster code or not, but the existence of
examples like [this one](https://godbolt.org/z/rK9v7sMdK) suggests that we're probably winning
at least some of the time.

Notice that my `std::swap_ranges`, like libc++ trunk's `std::copy` and `std::copy_backward`,
lacks a special case for ranges of size 1. That's [libstdc++ bug #108846](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108846);
whatever the libstdc++ folks do to resolve it, I expect that the libc++ folks will follow suit
within a month or two. So I felt no need to fix my fork's `swap_ranges` (and `copy` etc.) for
ranges of size 1, quite yet.
