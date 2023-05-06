---
layout: post
title: "Escape analysis hates copy elision"
date: 2021-03-07 00:01:00 +0000
tags:
  copy-elision
  cpplang-slack
  implicit-move
  pitfalls
  proposal
excerpt: |
  Yesterday Lauri Vasama showed me [this awesome Godbolt](https://godbolt.org/z/jG7x5h):

      struct S {
          static std::unique_ptr<S> make() noexcept;
          ~S();

          static std::unique_ptr<S> factory() {
              std::unique_ptr<S> s = S::make();
              return M(s);
          }
      };

      void take_ownership(S*) noexcept;

      void test() {
          std::unique_ptr<S> p = S::factory();
          take_ownership(p.release());
      }

  When `M(s)` is defined as `std::move(s)`, Clang gives pretty much optimal codegen
  for `test`:
---

Yesterday Lauri Vasama showed me [this awesome Godbolt](https://godbolt.org/z/jG7x5h):

    struct S {
        static std::unique_ptr<S> make() noexcept;
        ~S();

        static std::unique_ptr<S> factory() {
            std::unique_ptr<S> s = S::make();
            return M(s);
        }
    };

    void take_ownership(S*) noexcept;

    void test() {
        std::unique_ptr<S> p = S::factory();
        take_ownership(p.release());
    }

When `M(s)` is defined as `std::move(s)`, Clang gives pretty much optimal codegen
for `test`:

      pushq %rax
      movq %rsp, %rdi
      callq S::make()
      movq (%rsp), %rdi
      callq take_ownership(S*)
      popq %rax
      retq

But by defining `M(s)` as `std::move(s)`, we're actually creating a return statement
of the form `return std::move(s)`, which is widely known as an antipattern in C++.
Returning "by `move`" disables [RVO](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#rvo-nrvo-urvo),
and even when RVO doesn't happen, `return std::move(x)` is (in 99% of cases)
no more efficient than `return x`, because `return x` triggers a special case in the
standard known as "implicit move," which means that the copy into the return slot uses
the _move_ constructor, not the _copy_ constructor, even though the returned expression
`x` is totally an lvalue.

> I have a proposal before EWG right now —
> [P2266](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2266r0.html) —
> which in C++23 will make that `x` into an rvalue, literally. This radically simplifies
> the wording and compiler implementation around "implicit move."

Okay, so, let's get rid of that "return by `move`." Let's define `M(s)` to just `(s)`,
and see what happens. (Here's the [Godbolt](https://godbolt.org/z/jG7x5h) again.)

      pushq %rbx
      subq $16, %rsp
      leaq 8(%rsp), %rdi
      callq S::make()
      movq 8(%rsp), %rdi
      movq $0, 8(%rsp)
      callq take_ownership(S*)
      movq 8(%rsp), %rbx
      testq %rbx, %rbx
      je .LBB0_2
      movq %rbx, %rdi
      callq S::~S() [complete object destructor]
      movq %rbx, %rdi
      callq operator delete(void*)
    .LBB0_2:
      addq $16, %rsp
      popq %rbx
      retq

Whoa! The code got way longer! What happened?

----

What happened was _copy elision._ When the body of `factory` was this:

    static std::unique_ptr<S> factory() {
        std::unique_ptr<S> s = S::make();
        return std::move(s);
    }

the meaning was "Create an object named `s`, whose initial value comes from
being used as the return slot of `S::make()`. Then, construct the object
in my own return slot by calling `S`'s move constructor on xvalue `s`."

When the body of `factory` changed to this:

    static std::unique_ptr<S> factory() {
        std::unique_ptr<S> s = S::make();
        return s;
    }

the meaning changed as well, to "_Either_ do the same thing as above; _or_,
let `s` be another name for my return slot, and give it its initial value by
using it as the return slot of `S::make()`." In the latter case we've gone
from having "`s`" and "my return slot" be two separate objects, to having
them be one single object. This is normally a big performance win; we like it.

But in this case, this optimization interfered with Clang's _escape analysis_.
Escape analysis is the thing that tells us, in

    void f(int);
    int test() {
        int x = 42;
        f(x);
        return x + 1;  // i.e., return 43
    }

that `x` must still be 42 after the call to `f`, because
(even though we don't know what that function does in general) we know it
can't modify `x`, because it doesn't know `x`'s address on our stack.
Escape analysis tracks everything we do with `x`'s address, and can prove
in this case that that address has never _escaped_ into the wider world.

However, if you change `f` to take its parameter as `const int&`,
then `x`'s stack address _does_ escape, and so the compiler can't assume
the value of `x` remains unchanged after the call to `other_function`:
it must reload `x` from memory and actually compute that addition.
([Godbolt.](https://godbolt.org/z/751Yc3)) Because `f` might do

    void f(const int& x) {
        *const_cast<int*>(&x) = 918;
    }

Even worse, consider this caller:

    void g(const int&);
    void h();
    int test() {
        int x = 0;
        g(x);
        x = 42;
        h();
        return x + 1;
    }

In this case, `h` doesn't even receive `x`'s address... yet `h` can still
modify `x`, because by this point `x`'s address has already escaped! `g` and `h`
might collude together:

    int *global;
    void g(const int& x) {
        global = const_cast<int*>(&x);
    }
    void h() {
        *global = 918;
    }

The compiler's escape analysis cannot rule out this possibility, and so,
_for all the compiler knows,_ it might be the truth! After the call to `h`,
the compiler cannot assume that `x`'s value remains 42.

----

Back to our `unique_ptr` example.

    struct S {
        static std::unique_ptr<S> make() noexcept;
        ~S();

        static std::unique_ptr<S> factory() {
            std::unique_ptr<S> s = S::make();
            return M(s);
        }
    };

    void take_ownership(S*) noexcept;

    void test() {
        std::unique_ptr<S> p = S::factory();
        take_ownership(p.release());
    }

Escape analysis reasons as follows:

* The address of `factory`'s stack variable `s` escapes into `S::make`,
    as part of the calling convention for how `S::make` returns its prvalue
    result. We don't know what `S::make` does with that address.

* Later, we null out `test`'s stack variable `p`.

* Then, we call `take_ownership`. We don't know what it does.

* Finally, we destroy `p`, which is a no-op if and only if `p` is still null.

Could `take_ownership` affect `p`'s value, the way `h` affected `x`'s value in our
simpler example? Or can we assume that `p`'s value is definitely still null?
In short: does `p`'s address _escape?_

Well, `p` is just another name for the return slot of `S::factory`.
When `S::factory` returns "by `move`," it is constructing that object _right there_,
using the move-constructor of `unique_ptr`, which is fine for escape analysis
because that move-constructor is `inline` — we know what it does, and it _doesn't_
stash any addresses in global variables. So `p`'s address doesn't escape, which
means that `take_ownership` can't possibly have access to `p`, which means that
`p` is still null when `take_ownership` returns, which means that we don't need
to reload its value nor generate any code to call `delete` on it if it's non-null.

But, when `S::factory` returns "by name," copy elision kicks in. Now the returned
variable `s` becomes an alias for the object in `factory`'s return slot (which
you'll recall is already an alias for `p`). And `s`'s address _does_ escape —
it escapes into `S::make`!

Suppose `S::make` and `take_ownership` were secretly colluding, like this:

    std::unique_ptr<S> *global;

    std::unique_ptr<S> S::make() {
        std::unique_ptr<S> origp = std::make_unique<S>();
        global = &origp;
        return origp;
    }

    void take_ownership(S *rawp) {
        delete rawp;
        *global = std::make_unique<S>();
    }

The compiler cannot rule out this possibility, and so it must assume that calling
`take_ownership` might repopulate the stack variable `p` with a new value.
So it generates all that extra code in `test` to reload, check, and possibly `delete`
the value of `p`.

----

Clang and ICC behave pretty uniformly as described in this blog post.
GCC "succeeds" in generating good code for both variations of the Godbolt above (with
or without `std::move`), but this appears to be [due to a GCC bug](https://stackoverflow.com/questions/48749440/c-nrvo-copy-elision-with-return-statement-in-parentheses) —
it thinks `return (s)` with parentheses is a request to disable copy elision!
A slight tweak to my `M` macro, to remove the redundant parentheses,
and GCC joins the pack: [Godbolt](https://godbolt.org/z/361sfY).

----

It would be interesting to see a compiler patch that instrumented escape analysis somehow,
so that it could give an optimization-time note such as "Object `x`'s address escapes only
because copy elision in `f` gave `x` the same address as object `y`." I think that would happen
a lot, and certainly isn't something you'd want to see in your daily builds; but it might be
very interesting to trawl through the output.

----

Anton Zhilin's [P2025R1 "Guaranteed copy elision for return variables"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2025r1.html)
(June 2020) talks a bit about escape analysis in section 6.5, "What about the invalidation of optimizations?"
The more we expand the scope of copy-elision, the easier it will be for copy-elision to bump up against
escape analysis. The example dissected in today's blog post is interesting precisely _because_ it is
obscure and happens so rarely.

Please do not use this post as evidence that copy elision is bad! Copy elision is awesome!
Mainly this post is an interesting piece of trivia. But, secondarily, if any change to C++ _is_ needed
in this area, it would be _tightening up the object model_ so that escape analysis could
become more aggressive. I hope we all agree that the kind of "collusion" shown in this post is
terribly contrived, and nothing of value would be lost if C++ disallowed it. Escape analysis should
not allow for the possibility that a function has "remembered" the address of its own prvalue return
slot.

----

Previously on this blog:

* ["Downsides of omitting trivial destructor calls"](/blog/2018/04/17/downsides-of-omitting-trivial-destructor-calls) (2018-04-17)
