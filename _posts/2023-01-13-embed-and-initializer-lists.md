---
layout: post
title: 'P1967 `#embed` and D2752 "Static storage for `initializer_list`" are now on Compiler Explorer'
date: 2023-01-13 00:01:00 +0000
tags:
  initializer-list
  llvm
  parameter-only-types
  preprocessor
  proposal
  wg21
excerpt: |
  C++26 has adopted JeanHeyd Meneide's proposal [P1967 "`#embed`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1967r9.html),
  which will allow C++ (and C) programmers to replace their use of the [`xxd -i`](https://linux.die.net/man/1/xxd) utility
  with a built-in C preprocessor directive:

      unsigned char favicon_bytes[] = {
        #embed "favicon.ico"
      };
---

C++26 has adopted JeanHeyd Meneide's proposal [P1967 "`#embed`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1967r9.html),
which will allow C++ (and C) programmers to replace their use of the [`xxd -i`](https://linux.die.net/man/1/xxd) utility
with a built-in C preprocessor directive:

    unsigned char favicon_bytes[] = {
      #embed "favicon.ico"
    };

It supports some extra options to limit the number of bytes read, add headers and footers, and deal with
the need to suppress trailing commas for empty input:

    unsigned char favicon_bytes_plus_footer[] = {
      #embed "favicon.ico" limit(10000) suffix(,)
      0, 0, 0, 0
    };

This is a great feature! But it does have one downside: it makes it easier to run into the
surprising stack-frame cost of `std::initializer_list`.

## The surprising cost of `initializer_list`

Braced initializer lists _per se_ are extremely cheap — they're so transparent to the compiler that
they're not even considered "expressions" in C++'s grammar. (What is the type of `{1,2,3}`? Trick question:
it has none.) But in some contexts, a braced initializer can be used to initialize an object of type
`std::initializer_list<T>`; and _that_ is more expensive than you might think!

    void test(int i) {
      std::vector<int> v = {1,2,3};
    }

Here we're constructing a `std::vector<int>` object named `v`. Overload resolution selects the best-matching
constructor, namely `vector::vector(std::initializer_list<int>)`. The braced initializer list
`{1,2,3}` is used to initialize a `std::initializer_list<int>` object, which is then passed
to that constructor. Now, `initializer_list` is kind of like `string_view`: it's a reference-semantic,
"parameter-only" type. When you construct a `std::initializer_list<int>` object, it never holds the
elements `1,2,3` within itself; it simply refers to a contiguous range of elements stored somewhere else.
(It's very like C++20 `std::span<int>`; its only crime was being invented a decade too early.)

So if the elements `1,2,3` aren't stored inside the footprint of the `initializer_list` object, then where are
they stored? You might think "They're stored in global data, of course!" But that wouldn't
work in the general case, because we also want to be able to write

    std::vector<int> w = {i,i,i};

where `i` is known only at runtime, and could have a different value each time this line of code is hit.
So C++11 through C++23 define an `initializer_list` as referring to the elements of a temporary _backing array_
stored not in global data, but on the stack — just like any other temporary value. This is acceptable for short lists,
but a terrible idea when combined with `#embed` specifically!

    std::vector<unsigned char> getFavicon() {
      return {
        #embed "favicon.ico"
      };
    }

Suppose the data file "favicon.ico" consists of 2500 bytes. Then the executable containing `getFavicon()` will
contain those 2500 bytes somewhere (almost certainly in global data). When, at runtime, you call the function
`getFavicon()`, it will heap-allocate a 2500-byte array managed by the `vector` and copy that data into the
heap allocation — no surprise there. What _is_ surprising is that it will _first_ copy the data from global storage
onto the stack! See, it isn't actually "populating the `vector` with your data"; it's "constructing the `vector`
with a constructor that takes `initializer_list`." And that `initializer_list` must have a backing array; which
is a temporary, stored on the stack.

Now, LLVM's optimizer is good enough that it can notice the double-copy and short-circuit it, copying directly
from global storage into the heap, and won't even touch the stack pointer... if you turn on optimizations!
At `-O0`, you can actually see the stack pointer bumping down 2500 bytes.
An unwise programmer could `#embed` a million-byte file into a context requiring a `std::initializer_list`,
and boom, you've got yourself a million-byte stack frame.

Even at `-O2`, you can see the stack frame blow up. Just use a container that LLVM finds
a little less understandable, [such as `std::deque`](https://godbolt.org/z/4sTnhn1P4); or pass the `initializer_list`
[across an ABI boundary](https://godbolt.org/z/5dz6oso9E).

## The solution: "Static storage for braced initializers"

I've drafted a paper [D2752R0 "Static storage for braced initializers"](https://quuxplusone.github.io/draft/d2752-static-storage-for-braced-initializers.html)
that proposes to give implementations more freedom to place the backing arrays of `initializer_list` into static
storage, the way we do for string literals. Expect to see P2752 in the next WG21 mailing. (And if you'd
like to sign on as a coauthor, please let me know!)

I've implemented my proposal in Clang — in the same fork where I also maintain a working implementation of
[P1144 `[[trivially_relocatable]]`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1144r6.html) and a few
smaller papers. So you can play around with it on [Godbolt Compiler Explorer](https://p2752.godbolt.org/z/WjfqGqrPP).
Just remember to select the "Clang (experimental P1144)" compiler from the dropdown.

Since P2752 is motivated largely by P1967 `#embed`, I've gone ahead and implemented `#embed` for Clang as well.
That patch was [originally created by JeanHeyd Meneide](https://github.com/llvm/llvm-project/compare/main...ThePhD:llvm-project:dang)
but had bit-rotted to quite an extent already, so I fixed it up to conform to the proposal's latest revision
as I understand it. (Thanks to JeanHeyd for previewing a draft of this post!)

[Click here](https://p2752.godbolt.org/z/WsKT166dY) to see `#embed` and static braced initializers working in my Clang fork.

For the code, [see my GitHub](https://github.com/Quuxplusone/llvm-project/compare/main...p2752).
