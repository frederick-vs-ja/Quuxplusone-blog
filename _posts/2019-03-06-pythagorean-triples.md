---
layout: post
title: "Four versions of Eric's Famous Pythagorean Triples Code"
date: 2019-03-06 00:02:00 +0000
tags:
  coroutines
  metaprogramming
  ranges
  sufficiently-smart-compiler
excerpt: |
  Back in December, Eric Niebler made some waves in the C++ twitterverse with a post titled
  ["Standard Ranges"](http://ericniebler.com/2018/12/05/standard-ranges/) (2018-12-05).
---

UPDATE: Reddit points out that it wasn't fair of me to use `std::cout` in every case but the simplest one.
I've previously reported on the [surprisingly huge iostreams penalty](/blog/2018/04/27/pq-replace-top/#obvious-bottleneck-1-iostreams)
(2018-04-27).
So I've now appended numbers substituting `printf` for `cout` in each case below.

----

Back in December, Eric Niebler made some waves in the C++ twitterverse with a post titled
["Standard Ranges"](http://ericniebler.com/2018/12/05/standard-ranges/) (2018-12-05).

[Here is Eric's code](https://godbolt.org/z/BHODEL) ([backup](/blog/code/2019-03-06-triples-ranges.cpp)), modified to use the range-v3 library,
and with all the concepts/constraints removed since they use syntax that — while valid C++2a —
has never yet been implemented in either GCC or Clang. The constraints in his original code were there
just for documentation; they weren't being used for any functional reason, such as to control overload
resolution. Removing the constraints means the compiler doesn't have to check them,
so if anything it should compile *faster.*

This code is 74 lines long (or 66, if you remove the `using`-directives I added). It takes 10.5 seconds
to compile on Godbolt Compiler Explorer, and produces 715 lines of machine code.
You can [run it on Wandbox](https://wandbox.org/permlink/A463wD3MB6VIwZnO) and see that it produces
the correct answer.

([Printf version](https://godbolt.org/z/-QK7f3): 10 seconds to compile. 677 lines of machine code.)

----

[Here is a Coroutines version of the same code](https://godbolt.org/z/icY6RZ) ([backup](/blog/code/2019-03-06-triples-coro.cpp)).

This code is 53 lines long. It takes 2 seconds to compile on Godbolt Compiler Explorer, and
produces 437 lines of machine code.
You can [run it on Wandbox](https://wandbox.org/permlink/uZI03C6UkLP3GMl4) and see that it produces
the correct answer.

([Printf version](https://godbolt.org/z/rgl1ob): 0.8 seconds to compile. 394 lines of machine code.)

----

[Here is a C++17 callback-based version of the same code](https://godbolt.org/z/lHrSN2) ([backup](/blog/code/2019-03-06-triples-callback.cpp))

This code is 50 lines long. It takes 2 seconds to compile on Godbolt Compiler Explorer, and
produces 93 lines of machine code.
You can [run it on Wandbox](https://wandbox.org/permlink/eY20LSEEuE8VEn6f) and see that it produces
the correct answer.

([Printf version](https://godbolt.org/z/DceWlI): 1.1 seconds to compile. 58 lines of machine code.)

----

[Here is a C++17 "vanilla" version of the same code](https://godbolt.org/z/IH14zJ) ([backup](/blog/code/2019-03-06-triples-vanilla.cpp)).

This code is 16 lines long. [Its iostreams version](https://godbolt.org/z/iXRMuP) takes 1 second to
compile on Godbolt Compiler Explorer, and produces 87 lines of machine code.
You can [run it on Wandbox](https://wandbox.org/permlink/m02zKulIgyi488Td) and see that it produces
the correct answer.

(Printf version: 0.2 seconds to compile. 59 lines of machine code.)

----

Notice that all of these solutions to the Pythagorean Triples problem have the same essential form:
three nested `for` loops, with an `if` at the innermost level to do the filtering-out, and then a
stateful "counter" that goes from 1 to 10 and then bails out of the entire looping structure.

The callback-based and Ranges versions abstract out the "counter" into a filter named `take` that
fits into a general "pipeline" approach. My Coroutines approach promotes the counter to the top-level `for` loop,
and stuffs all the other logic into the generator (which is very similar to what Eric's Ranges version does).
My "vanilla" version treats the counter as a filter (i.e., an `if`) that naturally fits right next to the
other filter that checks for the Pythagorean-ness of each triple.

Each approach has its similarities and differences with the others, and its corresponding upsides and downsides.
All I would recommend is that whatever job you have to do, you should try to use the right tool for it. :)

----

UPDATE 2: A comment from Steven Wright alerts me to the fact that the Ranges version reifies each layer of
the nested loop as a separate lambda, which means that the compiler can't hoist constants through the loops.
(The hoisted constants would have to become new lambda captures, and changing the number of captures in a lambda
is an ABI-visible change that no compiler will ever be willing to make on its own.)

So you can achieve a runtime speedup (which I unscientifically measured at 11%)
simply by changing the original

    auto triples =
      for_each(iota(1), [](int z) {
        return for_each(iota(1, z+1), [=](int x) {
          return for_each(iota(x, z+1), [=](int y) {
            return yield_if(x*x + y*y == z*z,
              make_tuple(x, y, z));
          });
        });
      });

to manually hoist loop invariants out of the inner loops:

    auto triples =
      for_each(iota(1), [](int z) {
        return for_each(iota(1, z+1), [=, z2=z*z](int x) {
          return for_each(iota(x, z+1), [=, x2=x*x](int y) {
            return yield_if(x2 + y*y == z2,
              make_tuple(x, y, z));
          });
        });
      });

With the vanilla and callback-based versions, the compiler does this optimization for you automatically.
With the coroutines version, my impression is that the compiler *should* be able to do this optimization,
even though Clang currently does not.

----

UPDATE, 2024-05-10: Both Ranges and Coroutines are well enough supported these days that
I should update these programs to use Standard C++. In fact, C++23 has `std::generator`!
So here's some updated programs, and a table for comparison:

|----------------------|-------------------|------------------|
| Version              | 2019              | 2024             |
|----------------------|-------------------|------------------|
| C++2a range-v3       | [10 sec, 677 lines](https://godbolt.org/z/-QK7f3) | [7 sec, 472 lines](https://godbolt.org/z/-QK7f3) |
| C++20 Ranges         |                   | [3 sec, 345 lines](https://godbolt.org/z/7nWcYbofT) |
| C++2a coroutines     | [0.8 sec, 394 lines](https://godbolt.org/z/icY6RZ) | [3 sec, 426 lines](https://godbolt.org/z/hMb17Pzc1) |
| C++23 coro generator |                   | [2 sec, 272 lines](https://godbolt.org/z/WhYr64obr) |
| C++17 callbacks      | [1.1 sec, 58 lines](https://godbolt.org/z/DceWlI) | [0.3 sec, 61 lines](https://godbolt.org/z/DceWlI) |
| C++17 vanilla        | [0.2 sec, 59 lines](https://godbolt.org/z/IH14zJ) | [0.3 sec, 59 lines](https://godbolt.org/z/IH14zJ) |
