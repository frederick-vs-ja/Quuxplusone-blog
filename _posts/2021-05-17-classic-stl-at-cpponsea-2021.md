---
layout: post
title: '"Modern STL Programming" at C++ On Sea 2021'
date: 2021-05-17 00:01:00 +0000
tags:
  conferences
  cpp-on-sea
  stl-classic
  training
---

It's almost time for C++ On Sea 2021! This three-day all-remote conference runs
June 30th through July 2nd, 2021. This year it consists of just a single day
of conference presentations, preceded by two days of online "workshops" —
long-form half-day, full-day, or two-day classes. One of those two-day classes
will be Arthur O'Dwyer on "Modern STL Programming: Algorithms, Containers, Iterators."

You might be familiar with
CppCon's training model, where you buy a conference ticket and then add on
specific pre- and/or post-conference classes à la carte for about $1000 each.
In contrast, C++ On Sea 2021 offers a _combined_ conference-and-workshop ticket
which entitles you to attend all three days, including any class(es) you like;
and the combined ticket price is a shockingly low [£600](https://cpponsea.uk/tickets/) ($850)
for the whole three-day package.

The schedule of talks for the "conference day" has not yet been announced,
but judging from past years ([2020](https://cpponsea.uk/2020/schedule/),
[2019](https://cpponsea.uk/2019/schedule/)), I'd predict that there will be
about three tracks of about five talks each. (UPDATE, 2021-05-27:
[The schedule has been announced](https://cpponsea.uk/2021/schedule/)
as two tracks of five talks each, plus a plenary keynote.)

The schedule of classes and workshops also has not yet been announced (UPDATE:
[yes it has been](/blog/2021/05/27/cpponsea-schedule-announced/))
but I can say with confidence that _one_ of the
classes you'll be able to attend will be Arthur O'Dwyer's "Modern STL Programming:
Algorithms, Containers, Iterators."

This is a new and improved version of the "Classic STL" course I gave online
[at CppCon 2020](https://cppcon.org/class-2020-classic-stl/). To fit that three-day
course into C++ On Sea's two days, I've removed the speculative unit on C++20
and reduced the number of labs from nine to six; but to increase its practical
usefulness for industry programmers, I've managed to _add_ a unit on smart pointers,
including a lab exercise where the student is asked to upgrade a piece of code
from raw pointers to the appropriate kind(s) of smart pointers.

If you took "Classic STL" at CppCon 2020, this will be basically the same course;
you shouldn't expect much change. But that's also good news: If you took the
CppCon 2020 course and liked it, here's your chance to recommend it to your friends
and coworkers — for a low low price — and with a bonus unit on smart pointers!
[Act now!](https://cpponsea.uk/tickets/)


## Class description

With the arrival of C++20 Ranges, it's more important than ever to have a solid grasp
of classic C++ STL concepts: algorithms, containers, and iterators. We'll look at the
technical considerations that went into the design of the Standard Template Library,
and how those considerations have evolved in the two decades between C++98 and C++20.

After motivating the STL's core concepts of non-owning iterators and half-open ranges,
we’ll cover the different kinds of iterators in the STL, and look at the mechanisms
by which the STL distinguishes iterator capabilities and the kinds of optimizations
that it can perform. We'll do a deep dive on comparator-based algorithms such as `merge`
and `partial_sort`. We'll also cover library iterator types such as `move_iterator`,
`insert_iterator`, and `ostream_iterator`; and show the usefulness of classic utility
types such as `std::reference_wrapper`. Students will be asked to write and evaluate
their own STL-style algorithms, and practice using `std::priority_queue`.

On Day 2, we'll cover the sequence containers and their various semantic guarantees,
such as contiguity or iterator-stability. Then we'll cover the associative containers,
including the unordered containers. We'll present best practices for using the standard
containers with user-defined comparators and hashers. We'll also cover the
"particular skills" of various containers, such as how `std::list::sort`
differs from `std::sort`.

The course is bookended by units on two C++ library facilities that are not
"classic STL" but are critically important for any programmer: `string_view` and the smart pointers.

The course is organized as lectures interleaved with six hands-on lab exercises.
The lab exercises require a C++14-or-later compiler and either `make` or `nmake`.

### Prerequisites

Attendees should have basic to intermediate knowledge of C++11, including at least
one year of programming experience. This course focuses on the library, not the
language; it's assumed you will be able to keep up with offhand mentions of
language concepts such as "class template," "rvalue reference," and "lambda."

No special knowledge of C++14, C++17, or C++20 is required.

### Course outline

<b>Day 1</b>

* `string` and `string_view` (+ lab)
* iterators and the notion of an algorithm (+ lab)
* standard algorithms: sorting, heaps, comparators (+ lab)
* iterator adaptors and output iterators

<b>Day 2</b>

* proxy types, `std::ref`, `std::pair`
* sequence containers, iterator invalidation (+ lab)
* associative and unordered containers (+ lab)
* upgrading to smart pointers (+ lab)


### Informational email

It's still too early to fill in all the blanks on the pre-class informational email
you'll get when you sign up; but for an example of the kind of thing you should expect, see
["Classic STL at CppCon 2020"](/blog/2020/09/12/classic-stl-at-cppcon-2020/#dear-students) (2020-09-12).
