---
layout: post
title: 'Stopping the cascade of errors'
date: 2018-08-23 00:02:00 +0000
tags:
  compiler-diagnostics
  concepts
  metaprogramming
  pearls
  templates
---

In 2016, Andrzej Krzemieński
[pointed out](https://akrzemi1.wordpress.com/2016/03/21/concepts-without-concepts/) that
`static_assert` is great and all, but it doesn't stop the cascade of error messages
from something like this:

    template<class It>
    void sort2(It first, It last) {
        static_assert(is_random_access_iterator_v<It>);
        std::sort(first, last);
    }

He suggested a neat trick to suppress the cascade of error messages: simply
*don't compile* the call to `std::sort` unless the `static_assert` would have
passed. He did it with tag-dispatch; but in C++17 we can do it even more cleanly
with `if constexpr`. Example:

    template<class T>
    void sort2(It first, It last) {
        if constexpr (!is_random_access_iterator_v<T>) {
            static_assert(is_random_access_iterator_v<T>);
        } else {
            std::sort(first, last);
        }
    }

This second snippet yields exactly one error:

    error: static_assert failed due to requirement
    'is_random_access_iterator_v<int>'
            static_assert(is_random_access_iterator_v<T>);
            ^             ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    note: in instantiation of function template specialization
    'sort2<int>' requested here
        sort2(1, 2);
        ^
    1 error generated.

which doesn't seem impressive at all, until you compile the *first* snippet
and see its cascade of 16 errors, including such beauties as

    In file included from /opt/compiler-explorer/gcc-7.2.0/include/c++/7.2.0/algorithm:62:
    /opt/compiler-explorer/gcc-7.2.0/include/c++/7.2.0/bits/stl_algo.h:81:11: error: no matching function for call to object of type '__gnu_cxx::__ops::_Iter_less_iter'
          if (__comp(__a, __b))
              ^~~~~~
    /opt/compiler-explorer/gcc-7.2.0/include/c++/7.2.0/bits/stl_algo.h:1921:12: note: in instantiation of function template specialization 'std::__move_median_to_first<int, __gnu_cxx::__ops::_Iter_less_iter>' requested here
          std::__move_median_to_first(__first, __first + 1, __mid, __last - 1,
               ^
    /opt/compiler-explorer/gcc-7.2.0/include/c++/7.2.0/bits/stl_algo.h:1953:11: note: in instantiation of function template specialization 'std::__unguarded_partition_pivot<int, __gnu_cxx::__ops::_Iter_less_iter>' requested here
                std::__unguarded_partition_pivot(__first, __last, __comp);
                     ^
    /opt/compiler-explorer/gcc-7.2.0/include/c++/7.2.0/bits/stl_algo.h:1968:9: note: in instantiation of function template specialization 'std::__introsort_loop<int, int, __gnu_cxx::__ops::_Iter_less_iter>' requested here
              std::__introsort_loop(__first, __last,
                   ^
    /opt/compiler-explorer/gcc-7.2.0/include/c++/7.2.0/bits/stl_algo.h:4836:12: note: in instantiation of function template specialization 'std::__sort<int, __gnu_cxx::__ops::_Iter_less_iter>' requested here
          std::__sort(__first, __last, __gnu_cxx::__ops::__iter_less_iter());
               ^

----

In the comments, [Sergey Vidyuk suggests](https://akrzemi1.wordpress.com/2016/03/21/concepts-without-concepts/#comment-4798)
a neat (or maybe just "cute") hack if you want to use `enable_if` SFINAE
but don't like typing out `enable_if` or deciding where to put it in
your function's signature.  (Unlike the previous snippets, this does SFINAE
on failure, rather than erroring out.)

    template<class T,
        class = std::enable_if_t<is_random_access_iterator_v<T>>>
    using RandomAccessIterator = T;

    template<class T>
    void sort2(RandomAccessIterator<T> b, RandomAccessIterator<T> e)
    {
        std::sort(b, e);
    }

[Here's a Godbolt](https://godbolt.org/z/7piE-_)
comparing the error messages we get from each of the three snippets above.
The really cool thing is that Clang is smart enough to recognize our
Concepts-like idiom in the third snippet:

    error: no matching function for call to 'sort2'
        sort2(il.begin(), il.end());
        ^~~~~
    note: candidate template ignored:
        requirement 'is_random_access_iterator_v<_List_iterator<int> >'
        was not satisfied [with T = std::_List_iterator<int>]
    void sort2(RandomAccessIterator<T> b, RandomAccessIterator<T> e)
         ^
