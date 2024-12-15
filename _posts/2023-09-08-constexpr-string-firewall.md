---
layout: post
title: "Just how `constexpr` is C++20's `std::string`?"
date: 2023-09-08 00:01:00 +0000
tags:
  c++-learner-track
  constexpr
  cpplang-slack
  library-design
---

In C++20, `string` and `vector` are marked `constexpr`, which means they're _somewhat_
usable in compile-time programming. For example, we can write:

    constexpr std::string firstName(std::string s) {
      size_t n = s.find_first_not_of(' ');
      if (n == s.npos)
        return "";
      return s.substr(n, s.find(' ', n) - n);
    }
    constexpr std::string bard() {
      return "William Shakespeare";
    }
    static_assert(firstName(bard()) == std::string("William"));

and it will Just Work. But we can't write this:

    constexpr std::string s = "William Shakespeare";

What's the deal?

## Two constraints on constexprness

A `constexpr` variable's value (or a `constinit` variable's initial value)
must be known at compile time. Therefore, our first constraint is that the
value can't depend on any runtime input.

    int main(int argc, char**) {
      constexpr int n = argc; // Error
    }

> This snippet, and the next one, show `constexpr` stack variables.
> In real life, there's basically no reason ever to use `constexpr`
> on a stack variable; you'll use it only on globals or as part of the set phrase
> `static constexpr`. But C++ physically allows `constexpr` even on stack variables,
> so I use it here for simplicity.

C++ treats memory addresses just as you'd expect if you know about ELF files.
The addresses of variables in the static data section (i.e. globals and function-local statics)
are treated as compile-time constants (because we can encode them in a single ELF relocation);
but any non-trivial math on those addresses becomes non-constant.
And the address of a stack variable is automatically non-constant.

    int main() {
      int x = 0;
      static int y = 1;
      constexpr int *p = &x; // Error
      constexpr int *q = &y; // OK
      constexpr intptr_t r = intptr_t(q) * 47; // Error
    }

That is, C++ forbids data to flow "backward" from runtime back into compile-time.
You might say the laws of physics forbid that! How could we possibly use at compile time,
and encode into the data section, a numeric value that won't be known until runtime?

> Note that when I say "value," what the C++ compiler hears is "object representation" —
> the sequence of bytes that actually gets stored, even if that's not the object's "value"
> in the Platonic sense. This will become important later.

The second constraint is that C++ forbids certain data to flow "forward" from compile-time
into runtime. This constraint is slightly less obvious. Let's look at it:

## Constexpr allocation is fake allocation

Constexpr evaluation, ever since C++11, has always allowed us to do "stack allocation"
at compile time. This naïve `fib` function asks the compiler to allocate four bytes of
"stack memory" for each of `a` and `b` at the top level, and then again at the first level
of recursion, and again at the second level, and so on.

    constexpr int fib(int n) {
      if (n <= 1) return n;
      int a = fib(n - 1);
      int b = fib(n - 2);
      return a + b;
    }
    static_assert(fib(10) == 55);

If this were a runtime function, it would just generate code to bump the stack pointer at
runtime. But at compile time, there is no _actual_ stack; the compiler is just _pretending_
to run this code. The compiler somehow _pretends_ to have access to a stack segment at compile time.
This little lie goes undetected, as long as we confine our use of that "fake" stack segment
to compile time. But it goes bad if a fake-stack address "escapes" out into the actual runtime
program ([Godbolt](https://godbolt.org/z/47M97dh7G)):

    constexpr int *f() {
      int i = 42;
      return &i;
    }
    constexpr int *p = f(); // Error!
    int main() {
      printf("%p\n", (void*)p);
    }

This program tries to print out `f`'s return value (as evaluated at constexpr time), but
that would be a pointer into the constexpr evaluator's "fake" stack segment, which doesn't actually exist
at runtime. The compiler rejects this program: the variable `p` (which is accessible by `main` at
runtime) can't be initialized with a fake pointer value that doesn't actually exist at runtime.

C++20 extends this same "fake memory" allocation mechanism to include heap allocation.

The compiler's constexpr evaluator has no more access to the _actual_ runtime heap than it
has to the _actual_ runtime stack. It's still just pretending.
But again its lies go undetected, as long as we confine our use of the fake heap segment
to compile time. [Godbolt](https://godbolt.org/z/5EGqEWq54):

    constexpr auto g() {
      return std::make_unique<int>(42);
    }
    static_assert(*g() == 42); // OK
    constexpr int i = *g(); // OK
    constexpr bool gt = (g() != nullptr); // OK
    constexpr auto p = g(); // Error!

On the last line, we attempt to "escape" the fake-heap pointer result of `g` into a variable
`p` that has a real existence at runtime. That's not allowed; we get a compiler error instead.

## Subtleties of `string` and `vector`

Observe that merely _having_ a runtime variable of type `string` or `vector` counts as "escaping"
a pointer to its data. We can't write something like

    constexpr std::vector<int> v = {1, 2, 3};

because then we could try to print out the value of `(void*)v.data()` at runtime, and it would
be a pointer into the fake compile-time heap, and that's not allowed. But we _can_ say

    constexpr std::vector<int> v = {};

because an empty `vector` doesn't hold a pointer to a heap-allocation. There's nothing wrong
with "escaping" a null pointer from constexpr time into runtime!

### SSO matters

libstdc++ and Microsoft STL both reject

    constexpr std::string author = "William Shakespeare"; // 19 chars: Error!

but accept

    constexpr std::string author = "Shakespeare"; // 11 chars: OK

The former string contains a pointer to an allocated buffer of chars on the fake compile-time heap.
The latter string, being short enough to fit in [SSO](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#sbo-soo-sso), doesn't.

### Pointer-into-self matters

libstdc++ rejects the following code ([Godbolt](https://godbolt.org/z/1ErrKjdbq)),
while Microsoft accepts.

    int main() {
      static constexpr std::string abc = "abc"; // OK
      constexpr std::string def = "def";        // Error!
    }

Both are correct! The trick here is that libstdc++'s `std::string` (unlike Microsoft's)
always contains a pointer to its data, roughly like this:

    struct string {
      char *data_ = &sso_buffer_[0];
      union {
        char sso_buffer_[16];
        struct {
          size_t size_;
          size_t capacity_;
        };
      };
    };

So `def`'s object representation contains a pointer to the memory address of `def.sso_buffer_`,
which is located inside object `def` on the _actual_ runtime stack frame of `main`.
We're asking for `def`'s value to be computed at compile time; but that value (which the compiler
hears as "object representation") depends on `def`'s runtime address. That's not a
compile-time constant. Thus, failure.

On the other hand, `abc`'s object representation depends merely on `abc`'s runtime address,
which is statically known (as far as C++ is concerned) because `abc` is in static storage.
The compiler just generates a relocation to the address of `abc` (plus eight or whatever) and we're
good to go.

### libc++'s SSO size changes at compile time

> UPDATE: The following is no longer true of libc++ trunk!
> See ["`constexpr std::string` update"](/blog/2023/10/13/constexpr-string-round-2/) (2023-10-13).

[Trivial relocatability fans](/blog/2019/02/20/p1144-what-types-are-relocatable/) will be asking,
"What about libc++, whose `string` (like Microsoft's) involves no pointer-to-self? Can libc++ handle an example like `def`?"

Sadly, no. libc++ makes a decision here that probably
seemed like a good idea back in 2020 when constexpr `std::string` was first being implemented
and nobody really knew how it was going to develop, but which seems indefensible in hindsight.
libc++ uses [`if consteval`](https://en.cppreference.com/w/cpp/language/if#Consteval_if) to change
the SSO buffer capacity of `string` in constant-evaluation contexts from 23 chars down to zero chars.
So libc++ is physically _able_ to store short strings in a position-independent and thus `constexpr`-able
object representation; it just _chooses_ never to do so. This means that on libc++ (only), you
aren't even allowed to write

    constinit std::string s = "";

at global scope. libc++ implements that empty string as a fake heap allocation of one byte (for the
null terminator), and that's not `constinit`-able because it'd escape the fake pointer to runtime.

## Bottom line

The _intended_ use of constexpr `string` and `vector` is as local variables or return types of
constexpr or consteval functions, not as constexpr or constinit variables. Marking a `string`
or `vector` variable with the `constexpr` keyword is probably a bad idea. It can be done, but
the exact boundaries of what's accepted will vary among STL vendors.

----

See also:

* ["`constexpr std::string` update"](/blog/2023/10/13/constexpr-string-round-2/) (2023-10-13)
