---
layout: post
title: 'An argument _pro_ liberal use of `[[nodiscard]]`'
date: 2018-11-06 00:01:00 +0000
tags:
  c++-style
  nodiscard
  pitfalls
---

Consider the following C++03 code (recently written by a student in a class of mine):

    #include <iostream>
    #include <vector>

    int main() {
        std::vector<int> myVector;

        myVector.push_back(1);
        myVector.push_back(2);
        myVector.push_back(3);

        std::vector<int>::const_iterator it;
        for (myVector.begin(); it != myVector.end(); ++it) {
            std::cout << *it << std::endl;
        }
    }

[When you compile and run this program, you get a segfault.](https://wandbox.org/permlink/ffIlcRzuYt6rP1JV)

How long did it take you to spot the bug?

----

The original sin in this code snippet was actually the definition of variable `it`
without any initializer. If we'd written the definition right at the point of
initialization —

        typedef std::vector<int>::const_iterator CIT;
        for (CIT it = myVector.begin(); it != myVector.end(); ++it) {
            std::cout << *it << std::endl;
        }

— then there'd have been no bug. Even better, we could have used C++11's `auto`
or ranged `for` loop:

        for (auto it = myVector.begin(); it != myVector.end(); ++it) {
            std::cout << *it << std::endl;
        }

        for (auto&& elt : myVector) {
            std::cout << elt << std::endl;
        }

Lessons learned:

* C++11 _does_ make a lot of things safer.

* Not all segfaults are due to subtle arcane issues — every so often, it's the simple stuff that gets you.
