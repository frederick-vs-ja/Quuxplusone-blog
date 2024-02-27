---
layout: post
title: 'Remember the `ifstream`'
date: 2018-11-26 00:01:00 +0000
tags:
  classical-polymorphism
  concepts
  library-design
  pitfalls
  rant
excerpt: |
  Quick, what does this C++17 code print?

      void foo(const std::istream&) {
          puts("istream");
      }
      void foo(const std::ifstream&) {
          puts("ifstream");
      }
      int main() {
          std::fstream t;
          foo(t);
      }
---

Quick, what does this C++17 code print?

    void foo(const std::istream&) {
        puts("istream");
    }
    void foo(const std::ifstream&) {
        puts("ifstream");
    }
    int main() {
        std::fstream t;
        foo(t);
    }

([Wandbox.](https://wandbox.org/permlink/W18vsVNAL13NDu3e)) That's right, it prints "istream" —
the `fstream` class (input/output, file-based) is derived from `istream` (input, non-file)
and also from `ostream` (output, non-file) but not from `ifstream` (input, file-based) and
not from `ofstream` (output, file-based).

There is [no inheritance relationship](https://godbolt.org/z/EjfjKl)
between `fstream`, `ifstream`, and `ofstream`. But there *is* an inheritance relationship between
`fstream` and `iostream`. The spaghetti of C++98-era multiple inheritance looks like this:

![Classical spaghetti](/blog/images/2018-11-26-inheritance-spaghetti.png)

----

Quick, what does this C++2a Working Draft code print?

    void foo(Same<int> auto) {
        puts("exactly int");
    }
    void foo(Integral auto) {
        puts("integral");
    }
    void foo(Regular auto) {
        puts("regular");
    }
    int main() {
        int t = 42;
        foo(t);
    }

Okay, trick question. This code doesn't compile: out of `Same<int>`, `Integral`, and `Regular`, none
of them subsumes any of the others, so the overload resolution is ambiguous. (This is not a bad thing,
in this case! Nobody should be writing code like the above.)

The spaghetti of C++2a-era multiple subsumption looks like this:

![Postmodern spaghetti](/blog/images/2018-11-26-subsumption-spaghetti.png)

Just like an old-time cartographer, I have filled in the blank space on the west side of my map
with non-sequitur dragons; in this case [`RegularInvocable`](https://en.cppreference.com/w/cpp/concepts/Invocable).
(That's right, there is no relationship between `Regular` and `RegularInvocable` — the C++2a Ranges
proposal introduced the term `Regular` to the standard library and immediately overloaded it with
multiple unrelated meanings.)

To be fair, I haven't come up with an easy surprising example such as my `ifstream` example
above. The best I can do is stuff like

- `Same<T, int>` does not subsume `Integral<T>`

- `Same<T, int>` *does* subsume `Same<int, T>`(!), and vice versa

- `Same<T, Base>` does not subsume `DerivedFrom<T, Base>`

- `SwappableWith<T, T>` does not subsume `Swappable<T>`, nor vice versa

Concepts are a lot weirder than classical inheritance, because they're parameterized. As you can see
above, `Constructible<T>` is a different concept (with different "derived concepts")
from `Constructible<T, T>`. And so we can get extra weirdnesses such as

- `DerivedFrom<T, std::ifstream>` does not subsume `DerivedFrom<T, std::istream>`

----

So in conclusion: _Remember the ifstream!_ Resist designing libraries with many couplings;
resist implementing couplings on an ad-hoc basis. Today's cutesy "because-we-can" spaghetti graph
is tomorrow's "but-why-would-you?" spaghetti graph.
