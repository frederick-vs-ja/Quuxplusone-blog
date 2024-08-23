---
layout: post
title: "Boost.Interprocess, and `sg14::inplace_vector`"
date: 2024-08-23 00:01:00 +0000
tags:
  allocators
  data-structures
  proposal
  relocatability
  sg14
  stl-classic
excerpt: |
  I'm blogging this mainly so that I'll have a quick cut-and-pasteable starting point
  the next time I need to use `boost::interprocess::offset_ptr` as a motivating example
  for something. Many thanks to StackOverflow user "sehe" for [the great SO answer](https://stackoverflow.com/a/62647095/1424877)
  from which I distilled this code.

  But also see [the end of this post](/blog/2024/08/23/boost-interprocess-tutorial/#bonus-sg14inplace_vector)
  for a bonus about allocator-aware `inplace_vector`.
---

I'm blogging this mainly so that I'll have a quick cut-and-pasteable starting point
the next time I need to use `boost::interprocess::offset_ptr` as a motivating example
for something. Many thanks to StackOverflow user "sehe" for [the great SO answer](https://stackoverflow.com/a/62647095/1424877)
from which I distilled this code.

But also see [the end of this post](#bonus-sg14inplace_vector) for a bonus about
allocator-aware `inplace_vector`.

## Intro to Boost.Interprocess

Boost.Interprocess basically gives us a way to create a named "shared memory object" —
essentially a disk file that we `mmap` into two different processes at once — and then
we can allocate C++ objects in there, which will be visible to both processes.
Synchronization is left as an exercise for the user; I'm not going to talk about
synchronization or two-way communication in this post. For this post, we'll deal with
a non-concurrent program that first "pickles" a value into a shared memory object
on disk, and then, on a later run, reopens the shared memory object to retrieve
("unpickle") that value. Here's the code ([backup](/blog/code/2024-08-23-pickle-int.cpp)):

    #include <boost/interprocess/managed_shared_memory.hpp>

    // This is the name of the shared memory segment Boost will create for us.
    // On Posix systems, it's likely a disk file named "/tmp/boost_interprocess/PICKLE".
    #define SHM_SEGMENT_NAME "PICKLE"

    namespace bip = boost::interprocess;

    int main(int argc, char **argv) {
      if (argc == 3 && std::string_view(argv[1]) == "--pickle") {
        bip::shared_memory_object::remove(SHM_SEGMENT_NAME);
        auto segment = bip::managed_shared_memory(bip::create_only, SHM_SEGMENT_NAME, 10'000);
        int *p = segment.construct<int>("MY_INT")();
        *p = atoi(argv[2]);
        printf("Wrote %d to the shared memory segment (at RAM address %p)\n", *p, p);
      } else if (argc == 2 && std::string_view(argv[1]) == "--unpickle") {
        auto segment = bip::managed_shared_memory(bip::open_only, SHM_SEGMENT_NAME);
        auto [p, nelem] = segment.find<int>("MY_INT");
        assert(p != nullptr); // it was found
        assert(nelem == 1);   // and it's a single int object, not an array
        printf("Read %d from the shared memory segment (at RAM address %p)\n", *p, p);
      } else {
        puts("Usage:");
        puts("  ./a.out --pickle 42");
        puts("  ./a.out --unpickle");
      }
    }

The odd syntax `segment.construct<int>("MY_INT")()` asks Boost.Interprocess to create a new named object
in shared memory. (The name allows us to find it in the other process by calling `segment.find` with
that same name.) `construct` actually returns a callable; the second set of parentheses contains
arguments to be perfectly forwarded to the new `int` object's constructor. So we could write simply:

    int *p = segment.construct<int>("MY_INT")(atoi(argv[2]));

and skip the assignment on the next line.

On my laptop, when I compile and run this (against Boost 1.85.0), I see this:

    $ g++ -std=c++17 -W -Wall -O2 pickle.cpp
    $ ./a.out --pickle 42
    Wrote 42 to the shared memory segment (at RAM address 0x1004000e0)
    $ ./a.out --unpickle
    Read 42 from the shared memory segment (at RAM address 0x1022980e0)
    $ ./a.out --unpickle
    Read 42 from the shared memory segment (at RAM address 0x100d480e0)

Notice that the shared memory segment is loaded at a different virtual address each time,
but we're still able to get to our stored `int` essentially by magic, by looking up our
object's root name `"MY_INT"`.

## Dealing with pointers and containers

The above works fine for `int`, but it breaks down as soon as we try to store any more complicated
C++ object in the shared memory segment. For example, suppose we try to store a `std::vector<int>`.
That would look like this ([backup](/blog/code/2024-08-23-pickle-std-vector-oops.cpp)):

    // DANGER, WILL ROBINSON!
    int main(int argc, char **argv) {
      if (argc >= 3 && std::string_view(argv[1]) == "--pickle") {
        bip::shared_memory_object::remove(SHM_SEGMENT_NAME);
        auto segment = bip::managed_shared_memory(bip::create_only, SHM_SEGMENT_NAME, 10'000);
        std::vector<int> *p = segment.construct<std::vector<int>>("MY_INTS")();
        for (int i = 2; i < argc; ++i) {
          p->push_back(atoi(argv[i]));
        }
        printf("Wrote %zu elements to a vector in the shared memory segment (vector at %p, data at %p)\n", p->size(), p, p->data());
      } else if (argc == 2 && std::string_view(argv[1]) == "--unpickle") {
        auto segment = bip::managed_shared_memory(bip::open_only, SHM_SEGMENT_NAME);
        auto [p, nelem] = segment.find<std::vector<int>>("MY_INTS");
        assert(p != nullptr); // it was found
        assert(nelem == 1);   // and it's a single vector<int> object
        printf("Found a vector with %zu elements in the shared memory segment (vector at %p, data at %p)\n", p->size(), p, p->data());
        printf("Those elements are:\n");
        for (int i : *p) {
          printf("  %d\n", i);
        }
      } else {
        puts("Usage:");
        puts("  ./a.out --pickle 1 2 3");
        puts("  ./a.out --unpickle");
      }
    }

This will compile just fine; but at runtime, it'll produce very wrong answers.

    $ ./a.out --pickle 3 1 4
    Wrote 3 elements to a vector in the shared memory segment
        (vector at 0x104b640e0, data at 0x600003098030)
    $ ./a.out --unpickle
    Found a vector with 3 elements in the shared memory segment
        (vector at 0x1047240e0, data at 0x600003098030)
    Those elements are:
      0
      0
      0

Do you see the problem? The `vector` object itself is successfully stored in the shared memory segment,
but the `vector` object itself is just a class type with three scalar fields: size, capacity, and data pointer.
These scalars are faithfully stored, and faithfully retrieved: the second process retrieves the vector's `size` as `3`
and its `data` pointer as `0x600003098030`, just the same as they were in the first process. But in the first
process, the pointer `0x600003098030` pointed to three ints on the first process's heap; in the second process,
there's nothing at that address (if it's even part of the second process's heap at all).

To make this work, we need to make sure the vector allocates memory from the shared memory segment,
not from the current process's global heap. That is, the output we want is more like this:

    $ ./a.out --pickle 1 2 3
    Wrote 3 elements to a vector in the shared memory segment
        (vector at 0x104d880e0, data at 0x104d88110)
    $ ./a.out --unpickle
    Found a vector with 3 elements in the shared memory segment
        (vector at 0x102a340e0, data at 0x102a34110)

Notice those `data` pointers are now pointing into the segment, not onto the global heap.
But notice another important subtlety: In the second process, the virtual address of the vector object
has changed by 0x2354000. That means the virtual address of the _entire shared memory segment_ has changed
by that much. Which means that the virtual address of our `data` allocation has _also_ changed by that much!
In the "bad" output above, the vector stored a raw pointer with value 0x600003098030, and that pointer
was reloaded with the same raw value in the second process, even though the data had shifted.
To achieve the "good" output, we need the vector to store basically an _integer offset_ within the segment
itself, so that the virtual address to which it "points" will change along with the address of
the segment.

> See also the section of Boost's documentation on
> ["limitations."](https://www.boost.org/doc/libs/1_86_0/doc/html/interprocess/sharedmemorybetweenprocesses.html#interprocess.sharedmemorybetweenprocesses.mapped_region_object_limitations)

Boost.Interprocess solves both of these problems in one fell swoop, by providing a stateful allocator
named `bip::allocator<T, bip::managed_shared_memory::segment_manager>`.
(And [others](https://www.boost.org/doc/libs/1_86_0/doc/html/interprocess/allocators_containers.html#interprocess.allocators_containers.allocator_introduction.allocator),
but we won't need them here.)
`bip::allocator`'s nested `pointer` type is the [fancy pointer](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0773r0.html) type `bip::offset_ptr<T>`.
An STL container parameterized with this allocator will both use the segment as its source of memory and
represent pointers in memory as basically integer offsets.

> `offset_ptr` is a class type with one private data member `ptrdiff_t m_offset`.
> Its `T& operator*() const` returns `(T*)((char*)this + m_offset)`.
> This means that when you copy, move, or relocate an `offset_ptr` from one address to another,
> it's not a trivial operation: `offset_ptr` is one of the canonical _non-trivially relocatable_ types.

Recall that an allocator is a handle to a heap ([CppCon 2018](https://www.youtube.com/watch?v=IejdKidUwIg));
here the `segment_manager` is the heap. Just as in PMR `std::pmr::polymorphic_allocator<T>` is
implicitly convertible from raw `std::pmr::memory_resource*`, in BIP `bip::allocator<...>` is implicitly
convertible from raw `bip::managed_shared_memory::segment_manager*`. And just like PMR, `bip::allocator`
is "sticky": it never propagates "horizontally" via [POCCA, POCMA, or POCS](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#pocca-pocma-pocs-soccc).

When we work with Boost.Interprocess, we'll use the following C++ STL types:

    #include <boost/interprocess/allocators/allocator.hpp>
    #include <boost/interprocess/managed_shared_memory.hpp>
    #include <scoped_allocator>

    namespace bip = boost::interprocess;

    namespace shm {
      template<class T> using allocator =
        std::scoped_allocator_adaptor<bip::allocator<T, bip::managed_shared_memory::segment_manager>>;
      template<class T> using vector =
        std::vector<T, shm::allocator<T>>;
    }

Every time we construct a new `shm::vector` object, we'll use one of its _allocator-extended constructors_,
which take all the same arguments as the ordinary constructors plus an extra parameter of type `shm::allocator`.

    // BEFORE (WRONG):
    std::vector<int> *p = segment.construct<std::vector<int>>("MY_INTS")();

    // AFTER (CORRECT):
    shm::vector<int> *p = segment.construct<shm::vector<int>>("MY_INTS")(alloc);

You might have noticed one more wrinkle in the typedef above. We wrapped our `bip::allocator` in
`std::scoped_allocator_adaptor`. That's because we need it to propagate "downward." Remember that every time
we construct a `shm::vector`, we need to use the allocator-extended constructor. If we create a
`shm::vector<shm::vector<int>>` and then `.emplace_back` into it, the outer vector's `.emplace_back`
must call the allocator-extended constructor of `shm::vector<int>`, not the ordinary constructor.
The STL handles this by allowing the allocator to define its own `.construct` method... which
`bip::allocator` does not. Instead, `std::scoped_allocator_adaptor` adds exactly that functionality.
Whenever you construct an object through such an allocator, its `construct` method checks to see
whether the object under construction could take such an allocator itself, and if so, it'll append `*this`
to the constructor argument list. (Naturally, this is a simplification; the real behavior is
[slightly more complicated](https://en.cppreference.com/w/cpp/memory/make_obj_using_allocator).)

Here's a working Boost.Interprocess program ([backup](/blog/code/2024-08-23-pickle-shm-vector-of-strings.cpp)).
It even stores a vector of strings, which (because `string` itself will allocate) exercises the downward-propagation
functionality. Sadly neither libc++ nor libstdc++ natively support fancy pointers in their own `basic_string`
types — you'll get awful compile-time errors — so we use `boost::container::basic_string`, which guarantees support.

    #include <boost/container/string.hpp>
    #include <boost/interprocess/allocators/allocator.hpp>
    #include <boost/interprocess/managed_shared_memory.hpp>

    namespace bip = boost::interprocess;

    namespace shm {
      template<class T> using allocator =
        std::scoped_allocator_adaptor<bip::allocator<T, bip::managed_shared_memory::segment_manager>>;
      template<class T> using vector =
        std::vector<T, shm::allocator<T>>;
      using string =
        boost::container::basic_string<char, std::char_traits<char>, shm::allocator<char>>;
    }

    using VectorOfStrings = shm::vector<shm::string>;

    int main(int argc, char **argv) {
      if (argc >= 3 && std::string_view(argv[1]) == "--pickle") {
        bip::shared_memory_object::remove(SHM_SEGMENT_NAME);
        auto segment = bip::managed_shared_memory(bip::create_only, SHM_SEGMENT_NAME, 10'000);
        auto alloc = shm::allocator<int>(segment.get_segment_manager());
        auto *p = segment.construct<VectorOfStrings>("MY_STRINGS")(argv+2, argv+argc, alloc);
        printf("Wrote %zu elements to a vector in the shared memory segment (vector at %p, data at %p)\n", p->size(), p, p->data());
      } else if (argc == 2 && std::string_view(argv[1]) == "--unpickle") {
        auto segment = bip::managed_shared_memory(bip::open_only, SHM_SEGMENT_NAME);
        auto [p, nelem] = segment.find<VectorOfStrings>("MY_STRINGS");
        assert(p != nullptr); // it was found
        assert(nelem == 1);   // and it's a single object
        printf("Found a vector with %zu elements in the shared memory segment (vector at %p, data at %p)\n", p->size(), p, p->data());
        printf("Those elements are:\n");
        for (shm::string& s : *p) {
          printf("  %s\n", s.c_str());
        }
      }
    }

## Bonus: `sg14::inplace_vector`

This past week I implemented the proposed `inplace_vector<T, N, Alloc>` in
[Quuxplusone/SG14](https://github.com/Quuxplusone/SG14/?tab=readme-ov-file#allocator-aware-in-place-vector-future--c20).
This allows you to use `sg14::inplace_vector` with Boost.Interprocess.
[Here](/blog/code/2024-08-23-pickle-shm-inplace-vector-of-strings.cpp) is the same vector-of-strings example,
using `sg14::inplace_vector`. That is, we create an `shm::inplace_vector<shm::string, 10>`, which is able to
propagate the `shm::allocator` down to each of its elements.

    $ g++ -std=c++20 -W -Wall -O2 pickle.cpp
    $ ./a.out --pickle 1 2 3
    Wrote 3 elements to a vector in the shared memory segment
        (vector at 0x102fa80e0, data at 0x102fa80f0)
    $ ./a.out --unpickle
    Found a vector with 3 elements in the shared memory segment
        (vector at 0x1027480e0, data at 0x1027480f0)
    Those elements are:
      1
      2  2
      3  3  3

    $ ./a.out --pickle 1 2 3 4 5 6 7 8 9 10 11
    libc++abi: terminating due to uncaught exception of type std::bad_alloc: std::bad_alloc
    Abort trap: 6

Observe the key characteristic of `inplace_vector`: its `data` is stored directly within the footprint
of the class itself. The 16 bytes in between are occupied by the `inplace_vector`'s `shm::allocator` member
and its `size`.

> A defaulted `inplace_vector<T, N>`, like a defaulted `vector<T>`, won't spend any storage
> on its `std::allocator` member, because `std::allocator` is stateless.
> But remember, `shm::allocator` is stateful: it holds a handle to the shared memory segment.

The code dealing with the `inplace_vector`-of-`string`s ends up very clean:

    using VectorOfStrings = shm::inplace_vector<shm::vector<int>, 10>;
    auto *p = segment.construct<VectorOfStrings>("MY_STRINGS")(argv+2, argv+argc, alloc);

If we had only the C++26 draft standard's _current_ `inplace_vector<T, N>`, without an allocator parameter,
we'd have to write this instead:

    using VectorOfStrings = std::inplace_vector<shm::string, 10>;
    auto *p = segment.construct<VectorOfStrings>("MY_STRINGS")();
    for (int i = 2; i < argc; ++i) {
      p->emplace_back(argv[i], alloc);
    }

Notice that the latter basically requires us to emulate allocator-awareness in userspace:
we can't just deal with an `inplace_vector`, but have to keep track of `alloc` alongside our `inplace_vector *p`
and remember to always use them as a pair. If we ever forget to pass the `alloc` parameter to an `emplace`,
`insert`, or `push_back` operation, we'll have a serious bug.
In the former, better, approach — using `sg14::inplace_vector<T, N, Alloc>` — we just construct the `inplace_vector`
using the proper allocator to begin with, and then we can use it like an ordinary container:
it's basically a drop-in replacement for `vector`. This is a good thing, and we're proposing that
C++26's `std::inplace_vector` should do it too.
