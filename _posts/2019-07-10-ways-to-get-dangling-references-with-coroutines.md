---
layout: post
title: 'C++2a Coroutines and dangling references'
date: 2019-07-10 00:01:00 +0000
tags:
  coroutines
  parameter-only-types
  pitfalls
---

Today I learned yet another way to generate accidentally dangling references with C++2a Coroutines.

## Boring old way

First of all, here's the boring old way that I'm sure everyone's aware of by now:

    generator<char> explode(const std::string& s) {
        for (char ch : s) {
            co_yield ch;
        }
    }

    int main() {
        for (char ch : explode("hello world")) {
            std::cout << ch << '\n';
        }
    }

([Run it on Godbolt!](https://coro.godbolt.org/z/Z6iNru))

See, in C++2a-Coroutine-world, the function `explode` isn't really one indivisible routine. It's a _coroutine_,
which means it gets split up by the compiler into a bunch of little code fragments — with the divisions between
fragments coming at so-called _suspend points_ — and the only way these fragments communicate with each other
is via _state_ stored in the _coroutine frame_.

What state is stored in `explode`'s coroutine frame? Well, it needs `ch` (or let's pretend it does for the
sake of this example); and it needs `s`. What's the type of `s`? `s` is a `const std::string&`. It's a reference
to the temporary string created on line 1 of `main()`. And when does that string die? Also on line 1 of `main()`!
So the reference captured into `explode`'s coroutine frame is dangling as soon as `explode` hits its first
suspend point.


## Boring old way, redux

Let's fix that bug. Local variables are preserved by-value in the coroutine frame. So let's copy `s`
into a local variable ASAP! ([Godbolt.](https://coro.godbolt.org/z/InuSdi))

    generator<char> explode(const std::string& s) {
        std::string copy = s;
        for (char ch : copy) {
            co_yield ch;
        }
    }

Does this fix the bug? You wish. See, `explode`'s first suspend point isn't at that `co_yield`.
Its first suspend point is `initial_suspend`, which isn't visible in the code! So by the time the
coroutine gets around to executing `std::string copy = s;`, the reference `s` has already gone dangling.


## Boring old way, redux2

Notice that we would have the same kind of issue if we used any "reference-semantic" type. It doesn't
have to be a native reference.

    generator<char> explode(std::string_view sv) {
        for (char ch : sv) {
            co_yield ch;  // Same bug!
        }
    }


## Exciting new way to dangle a reference

Okay. Now let me show you the one I just learned on Slack today. (Hat tip to mikezackles, Lewis Baker,
and Mathias Stearn!) ([Godbolt.](https://coro.godbolt.org/z/tMaVXY))

    generator<char> explode(const std::string& s) {
        return [s]() -> generator<char> {
            for (char ch : s) {
                co_yield ch;
            }
        }();
    }

    int main() {
        for (char ch : explode("hello world")) {
            std::cout << ch << '\n';
        }
    }

This version uses an immediately invoked lambda expression (IILE — we love our
[acronyms](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#iile)!) where the
body of the lambda is itself a coroutine (because it uses `co_yield`). In this version, `explode` is
not a coroutine; it's just a plain old subroutine.

- You call `explode`.

- `explode` constructs a lambda object.

- `explode` invokes that lambda's `operator()` (which btw is a coroutine).

- `explode` destroys that lambda object.

- `explode` returns to its caller.

And what happens to the `s` that is being used by the body of that coroutine? It's destroyed along with
the rest of the lambda object, at the closing curly brace of `explode`! By the time you're back in
`main()`, trying to step through the characters of that string, it's a dangling reference.

C++2a Coroutines are super sneaky. They'll slip in a dangling reference just about anywhere.


## Under construction

There are some active proposals out there that recognize the dangling-reference problem
with C++2a Coroutines; specifically I'm thinking of [P1063 "Core Coroutines"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1063r2.pdf)
and [P1342 "Unifying Coroutines TS and Core Coroutines."](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1342r0.pdf)

Lewis Baker's [P1745 "Coroutine changes for C++20 and beyond"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1745r0.pdf)
and of course Antony Polukhin's [P1485 "Better keywords for the Coroutines TS"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1485r0.html)
(previously discussed [here](/blog/2019/06/26/pro-p1485/)) are also in the general space of "recent exploration of
the Coroutines space," unrelated to the dangling-reference issues.

I expect that even if none of these changes get into the Working Draft, it will take at least six months for the community
to find all the biggest problems with Coroutines. Once the problems are found, it'll take another year to propose
and discuss solutions. And those solutions may themselves result in _new_ serious problems — it'll take another six
months after that even to be confident if they _don't_ cause new issues.

I see the same cycle of "heroic paper — oops — second heroic paper — oops —" playing out with `operator<=>`
at the moment. The previous cycle was [P1185](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1185r2.html) (adopted
in Kona). The current cycle is [P1186](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1186r2.html)
and [P1614](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1614r1.html) (likely to be adopted in Cologne,
if I understand correctly). The upcoming cycle is [P1630](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1630r0.html)
(and whatever happens with NTTPs; see ["Enums break strong structural equality"](/blog/2019/07/04/strong-structural-equality-is-broken/)
(2019-07-04) and Jorg Brown's [P1714](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1714r0.html)).

> UPDATE, 2022-10-16: The latest cycle was [P2468 "The Equality Operator You Are Looking For"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2468r2.html)
> (Revzin et al, January 2022), which fixed some issues introduced by [P1185](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1185r2.html)
> (Revzin, February 2019).

It's one thing to say "the perfect is the enemy of the good" and "sometimes worse is better."
It's quite another thing to send the ship to sea _while it is still actively being built._
Now, maybe the proper alternative in this ship metaphor is to wait until the last minute, then
vote the workmen off the ship, quickly verify that they've taken all their dropcloths and things with them,
and send the ship right on out. But I feel like it would be more productive, and even safer,
to re-schedule the launch with enough time that not only might the workmen finish their jobs,
but the ship's passengers might thoroughly _verify_ that the work has been completed and the dropcloths removed.
