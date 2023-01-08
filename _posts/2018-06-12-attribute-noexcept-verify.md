---
layout: post
title: 'Attribute `noexcept_verify`'
date: 2018-06-12 00:01:00 +0000
tags:
  compiler-diagnostics
  exception-handling
  proposal
---

Today on Slack, people were discussing the ever-popular `noexcept(auto)`.
The idea is that just as C++14 let us stop writing

    template<class T, class U>
    auto plus(T t, U u)
        -> decltype(t + u)
    {
        return t + u;
    }

and start writing simply

    template<class T, class U>
    auto plus(T t, U u)
    {
        return t + u;
    }

then wouldn't it be nice if some future version of C++ let us stop writing

    template<class T, class U>
    auto plus(T t, U u)
        noexcept(noexcept(t + u))
    {
        return t + u;
    }

(remember, [the number of the `noexcept`s shall be two](/blog/2018/04/09/long-long-long-is-too-long-for-gcc/))
and start writing simply the following?

    template<class T, class U>
    auto plus(T t, U u) noexcept(auto)
    {
        return t + u;
    }


## The upside

The *nice* thing about `noexcept(auto)` (which is not a real active proposal at the moment;
it's just something people have been talking about for about a decade now) is that it
would save us from rogue calls to `std::terminate`. Consider the C++17 code:

    template<class T>
    auto frobnicate(T t)
        noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        return t;
    }

    static_assert(noexcept(frobnicate(42)));
    static_assert(not noexcept(frobnicate(std::string{})));

Looks sane, right? But consider [calling it like this](https://wandbox.org/permlink/dS1bEYuVSmE9r8FB):

    struct Evil {
        Evil() = default;
        Evil(const Evil&) = default;
        Evil(Evil&) { throw "ha"; }
    };

    int main()
    {
        frobnicate(Evil{});
    }

It just calls `std::terminate`... because we got our `noexcept` condition wrong!
We meant to ask whether `noexcept(T(t))`. (And sure,
[that bugfix looks simple in hindsight](http://www.thesuccessfulcontractor.com/coachs-corner-a-plumbing-story/).
But remember, this was a simple contrived example. Getting it right in real code,
and getting it right 100% of the time, is not only difficult but practically
a fool's game.)

----

UPDATE: David Stone points out that even the "expert" (me) got it wrong!  `noexcept(T(t))`
tests the noexceptness of an *explicit* conversion, whereas what we actually did in
`return x;` was an *implicit* conversion. [Here's an `EvenMoreEvil`](https://wandbox.org/permlink/NjKAikfnU50H7fit)
that still calls `std::terminate` even after our supposed "fix."

----

If we were able to write this instead:

    template<class T>
    auto frobnicate(T t) noexcept(auto)
    {
        return t;
    }

then `frobnicate` would be noexcept when it could be, and non-noexcept when it needed to be.
Nice!


## The downside

While I love the *idea* of `noexcept(auto)`, I would oppose any serious attempt to push it into
the C++ Standard until we've seen that it can actually be implemented and used — *correctly*.
Consider this simple code:

    template<class T>
    auto frotz(T t) noexcept(auto)
    {
        puts("testing");
        return t;
    }

Under which circumstances is this function `noexcept`?

Well, it turns out that it's *never* noexcept. Because the entire C library is _nothrow_,
but none of it is _noexcept!_ (And no, spelling it `std::puts` will not change anything
in this department.)

So, adding a single `puts` or `printf` — or `memcpy` or `strlen` or `abort` — to your
`noexcept(auto)` function can instantly render it "non-`noexcept`" from the compiler's
point of view. Which might cause library pessimizations (such as how
[`vector<T>` reallocation falls off a cliff](/blog/2022/08/26/vector-pessimization/) if you omit `noexcept` from `T`'s move-constructor)
or might even cause `static_assert` failures in far-removed client code.

And the problem is not limited to the C library: there are actually lots of APIs
in the C++ standard library that [you might naïvely expect to be `noexcept`, but
they're not](/blog/2018/04/25/the-lakos-rule). For example:

    template<class T>
    struct Holder {
        T m_data;
        T exchange(T value) const noexcept(auto) {
            return std::exchange(m_data, value);
        }
    };
    Holder<int> h;
    static_assert(not noexcept(h.exchange(42)));  // yikes!

I don't think we'll ever get `noexcept(auto)` in the C++ Standard unless somebody does
the hard work to show concrete evidence (not just papers, but actual real-world usage)
that this problem is surmountable or mitigable somehow.


## The bright idea

The problem with `noexcept(auto)`, outlined above, boils down to: `noexcept(auto)` instructs
the compiler to take the wheel. For this to work, the compiler has to be a dang good
driver — it has to guess correctly for each function whether we intended it to be
`noexcept` or not. As we saw in the first example, humans are *terrible* at this game —
but as we saw in the second example, compilers seem *also* to be terrible at it.
We can't hand the wheel to the compiler unless we know the compiler is going to drive right.

So, [trust, but verify](https://en.wikipedia.org/wiki/Trust,_but_verify)? Suppose we had
a vendor-specific attribute — let's for the sake of argument call it `[[clang::noexcept_verify]]` —
which would instruct the compiler to compute the same exact thing as `noexcept(auto)`, but
then, *instead of applying it blindly to our function's signature*, the compiler would
merely look for an existing (possibly defaulted) `noexcept` specification on our function
and *verify* that the computed noexcept-ness matched the noexcept-ness expressed by our
existing specification!

The compiled program would still use the noexcept-ness expressed by the programmer.
The computed noexcept-ness would be used *only* for diagnostics; and the diagnostics
would not necessarily have to be fatal. (They could be dialed down to warnings.)

This would turn our "rogue `std::terminate`" example above into a compile-time failure.

    template<class T>
    [[clang::noexcept_verify]]
    auto frobnicate(T t)
        noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        return t;
    }

    int main()
    {
        frobnicate(Evil{});
    }

    warning: computed noexcept-specification 'noexcept(false)' does not match
    explicit noexcept-specification 'noexcept(true)' [-Wnoexcept-verify]
        frobnicate(Evil{});
        ^
    note: previous noexcept-specification is here
        noexcept(std::is_nothrow_copy_constructible_v<T>)
        ^

Such an attribute might see significant adoption — or at least experimentation.
It would still suffer from the same problem with C library functions; but now the symptom
of that problem would be bogus diagnostics at compile-time, instead of quietly turning
functions `noexcept(false)` when we didn't intend it. This would remove the *risk*
of asking for the compiler's help; and so it should lead to quick adoption, especially
by template libraries.

Finally, another benefit of the "trust but verify" approach is that it would quickly
yield empirical data on how often human programmers actually *do* get their `noexcept` conditions right.
For non-constant conditions (i.e. excluding cases of literal `noexcept(true)` and `noexcept(false)`),
I bet the answer is "less than 20% of the time."

I have *not* implemented this attribute. Frankly, I wouldn't know where to start with it.
But if you like the idea and want to give it a shot... please feel free!
