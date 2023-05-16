---
layout: post
title: 'Ways C++ prevents you from doing X'
date: 2020-03-17 00:01:00 +0000
tags:
  access-control
  c++-learner-track
  pitfalls
---

C++ has many orthogonal ways of preventing you from doing what you asked for. When I'm doing
training courses, I often find students confused (without necessarily knowing they're confused)
between these various failure modes. So I thought I'd write down the most common ones in a blog post.


## You can't modify it because it's `const`

    void read_it(int);
    void write_it(int&);

    struct S {
        int m;
        void foo() const;
    };

    void S::foo() const {
        m = 42;  // ERROR: can't modify a const object
    }

    const S s = {42};
    read_it(s.m);
    write_it(s.m);  // ERROR: call would lose const qualifier

For more on this subject, see ["`const` is a contract"](/blog/2019/01/03/const-is-a-contract/) (2019-01-03).


## You can't name it because it's inaccessible

    class C {
    public:
        int pub;
        int *getpriv() { return &priv; }
    private:
        int priv;
    };

    C c;
    read_it(c.pub);
    write_it(c.pub);
    read_it(c.priv);   // ERROR: inaccessible
    write_it(c.priv);  // ERROR: inaccessible

In this case, the problem is that the member `priv` is _private_ — it's not accessible in contexts that
aren't either members or friends of class `C`. The compiler is basically saying that you can't _name_ `c.priv`.

You can't use the _name_ of `c.priv`, but you can still modify the _object_ denoted by `c.priv`, if you can
get a reference to it by some other approach. For example, the class can expose a public API `getpriv()` that
returns a pointer to the private data member.

    read_it(*c.getpriv());  // OK
    write_it(*c.getpriv());  // OK

Access control affects your ability to name the private member, regardless of what you're
planning to do with it. If it's inaccessible, you can't read it _or_ write it.


## You can't see it because you didn't include the right header

    int i = f();      // ERROR: 'f' was not declared in this scope
    C *c;             // ERROR: 'C' does not name a type
    std::set<int> s;  // ERROR: no template named 'set' in namespace std
    boost::regex rx;  // ERROR: 'boost' does not name a type; did you mean 'bool'?

In C++, everything you use — functions, variables, types, type aliases, namespaces — must be _declared_
before use. Without a declaration telling it that `C` is a class type, or that `std::set` is a class template,
the compiler cannot figure out how to use those names at all.

So, you have to `#include <vector>` before you try to use `std::vector`.

The standard library has no particular organizing principle — it evolved over decades — so some of
its header names are hard to remember. You have to `#include <algorithm>`
before you use (some overloads of) `std::swap`. You have to `#include <memory>` before you use
`std::unique_ptr`.

> Starting in GCC 9, GCC will no longer attempt to "auto-correct" the identifiers [`boost`](https://github.com/boostorg/boost)
> and [`bsl`](https://github.com/bloomberg/bde/blob/04a94fe1321b739eff5bc7dd221f89c6ab885304/groups/bsl/doc/bsl.txt)
> to `bool`. Prior to GCC 9, if you see an error message ending in "...did you mean `bool`?", it's
> a sure bet that you forgot to include some library header.

One special case to be aware of:

### Right header, wrong language version

    #include <optional>
    std::optional<int> o;

All library vendors these days have converged on guarding _features_, not _headers_.
(This makes sense: they can hide a feature in source code behind `#if`, but they can't
hide the header file itself from the filesystem.) So if you compile the snippet above
with `-std=c++11` or `-std=c++14`, libc++ will tell you it doesn't know what `optional` is —
and libstdc++ will tell you it doesn't even know what `std` is! That's because the whole
contents of `<optional>` are hidden behind `#ifdef`s. That header was introduced in C++17,
so, pass `-std=c++17` to the compiler and the code will work as you intended.

## Its real name is more qualified than what you wrote

    #include <vector>

    vector<int> v;  // ERROR: 'vector' does not name a type

Remember, C++ doesn't have "modules" or "packages" in the Python or Java sense. We have headers _and_
we have namespaces, and they are orthogonal. Using `#include <vector>` to bring the declaration of
`std::vector` into scope does not entitle you to refer to that type as simply `vector`.

A `using`-declaration can provide a shorter name for a previously declared type — but, vice versa,
a `using`-declaration alone does not cause the original declaration to become visible.

    using std::vector;  // ERROR: 'vector' has not been declared
    vector<int> v;

You need to `#include` the proper header first; _then_ you can use `using` to create shorter synonyms
if you want. (But, generally speaking, you should not want.)


## The linker can't find its implementation

    main.o: In function `main':
    main.cpp:4: undefined reference to `foo()'
    collect2: error: ld returned 1 exit status

Even if you've included the right headers, `using`’ed the right namespaces, qualified all your names correctly,
and the compiler is happy, you might find that the linker is unhappy with your program. This is often because
one of your .cpp files calls a function which is declared, but never defined. Maybe it's defined
in `libfoo.a`, but you forgot to pass `-lfoo` to the linker. Maybe it's defined in `foo.o`, but you forgot to
add `foo.cpp` to your project. Maybe you legitimately forgot to write its definition _anywhere_.

Two special cases to be aware of:

### The undefined entity is a `static const` data member

Static data members are just global variables with funky names;
they must be defined somewhere out-of-line. Static _const_ data members are schizophrenic: for many purposes they
behave like `constexpr` compile-time constants, but if you ever take the address of a static const data member
(even by taking a reference to it), then you'll need an out-of-line definition again. [Example:](https://godbolt.org/z/CqVY8w)

    void byvalue(int);
    void byref(const int&);

    struct S {
        static constexpr int one = 1;
        static const int two = 2;
        void foo();
    };

    void S::foo() {
        byvalue(one);  // OK
        byvalue(two);  // OK
        byref(one);    // OK
        byref(two);    // LINKER ERROR: `two` is undefined
    }

Add an out-of-line definition to one of your .cpp files — or, better, use `static constexpr int` instead.
In C++17, `constexpr` static data members don't need out-of-line definitions.
(In C++17 and later, you could even use `static inline const int`; but I see no advantage to `static inline const`
over `static constexpr`.)

For more on this subject, see ["Why do I get a linker error with `static const` and `value_or`?"](/blog/2020/09/19/value-or-pitfall/) (2020-09-19).


### The undefined entity is a vtable

If you see an undefined reference to "vtable for SomeClass," [like this:](https://godbolt.org/z/nLybBN)

    main.o: In function `main':
    main.cpp:4: undefined reference to `vtable for Foo'
    collect2: error: ld returned 1 exit status

then you might wonder how you'd ever provide an out-of-line definition for the vtable. That's the compiler's
responsibility, isn't it? Well, according to
[the most common ABI for C++](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#vague-vtable),
the compiler always emits the vtable in the same .o file as the definition of the first virtual member function
of the class. (Not counting inline functions and pure virtuals, of course.) So if you see this kind of error,
you should act as if the linker were complaining about the first virtual function in the offending class.
Did you implement it? Did you link that .cpp file into your project?

For non-virtual member functions, it's normally perfectly fine to declare member functions that you never define;
but for virtual member functions, you must provide a definition for every declaration — no exceptions!
