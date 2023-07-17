---
layout: post
title: "Beware CTAD on `reverse_iterator`"
date: 2022-08-02 00:01:00 +0000
tags:
  class-template-argument-deduction
  pitfalls
  stl-classic
  war-stories
excerpt: |
  Consider the following example of an "STL-style algorithm,"
  taken from a lab exercise in one of my training courses:

      template<class It>
      bool is_palindrome(It first, It last) {
          while (first != last) {
              --last;
              if (first == last) break;
              if (*first != *last) {
                  return false;
              }
              ++first;
          }
          return true;
      }

  A palindrome is equal to itself when read forwards or backwards,
  so, I said blithely, we could rewrite it like this:

      template<class It>
      bool is_palindrome(It first, It last) {
          return std::equal(
              first, last,
              std::reverse_iterator(last),
              std::reverse_iterator(first)
          );
      }

  But look out — this code has a bug that causes undefined behavior!
  Do you see it?
---

Consider the following example of an "STL-style algorithm,"
taken from a lab exercise in one of my training courses:

    template<class It>
    bool is_palindrome(It first, It last) {
        while (first != last) {
            --last;
            if (first == last) break;
            if (*first != *last) {
                return false;
            }
            ++first;
        }
        return true;
    }

A palindrome is equal to itself when read forwards or backwards,
so, I said blithely, we could rewrite it like this:

    template<class It>
    bool is_palindrome(It first, It last) {
        return std::equal(
            first, last,
            std::reverse_iterator(last),
            std::reverse_iterator(first)
        );
    }

But look out — this code has a bug that causes undefined behavior!
Do you see it?

----

Here's a test case that triggers the undefined behavior ([Godbolt](https://godbolt.org/z/P1svYq35q)):

    std::string s = "foo";
    assert(!is_palindrome(s.begin(), s.end()));
    assert(is_palindrome(s.begin()+1, s.end()));
    assert(!is_palindrome(s.rbegin(), s.rend()));
    assert(is_palindrome(s.rbegin(), s.rend()-1));  // Fails! Oof!

The bug is that I left the five characters `make_` out of
`std::make_reverse_iterator`. (Or, alternatively, I left the four
characters `<It>` out of `std::reverse_iterator<It>`.) When I
wrote `std::reverse_iterator(last)` without angle brackets,
I was accidentally triggering C++17 class template argument
deduction (CTAD). CTAD tries to _deduce_ the template arguments
to `std::reverse_iterator`; and in the latter two cases above,
it sees that `last` is already a `reverse_iterator` (specifically
`reverse_iterator<string::iterator>`). So CTAD thinks it
doesn't need to do anything.

Thus, in the latter two cases above, `std::reverse_iterator(last)`
is treated as just another way of writing `last`, and so we're
actually executing `std::equal(first, last, last, first)` — which
immediately runs off the end of the string and has undefined behavior.

My recommended fix is never to use CTAD (_certainly_ never in generic code).
I continue to wish that any mainstream compiler would add an opt-in `-Wctad` diagnostic;
see ["_Contra_ CTAD"](/blog/2018/12/09/wctad/) (2018-12-09) and
["Measuring adoption of C++17 and CTAD in real codebases"](/blog/2019/01/16/counting-ctad/) (2019-01-16).
(I [submitted such a diagnostic to Clang](https://reviews.llvm.org/D54565) back in November 2018,
but it never made progress.)

----

The specific case of `reverse_iterator` has come up for others in the past:

- [P1021 "Extensions to Class Template Argument Deduction"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1021r0.html) (Mike Spertus, May 2018)

- [P0896R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0896r3.pdf)'s specification of `reverse_view` had exactly this bug;
    it was fixed in R4 (Eric Niebler, October 2018)

- ["Is `std::make_move_iterator` redundant...?"](https://stackoverflow.com/a/57762216/1424877) (Barry Revzin, September 2019)

----

Separately, it's mildly interesting that this is an algorithm whose performance
can be improved (at least in debug mode) by _not_ using the C++14 four-argument
version of [`std::equal`](https://en.cppreference.com/w/cpp/algorithm/equal).

    template<class It>
    bool is_palindrome(It first, It last) {
        return std::equal(
            first, last,
            std::reverse_iterator<It>(last)
            // do NOT pass the end of the second range
        );
    }

The four-iterator version can be O(1) in the case that all the iterators are
random-access and `(last1 - first1) != (last2 - first2)`:
different-length ranges can never be "equal."
But in this case we know our ranges are definitely the same length,
and so we can save a bit of time by skipping that comparison.
