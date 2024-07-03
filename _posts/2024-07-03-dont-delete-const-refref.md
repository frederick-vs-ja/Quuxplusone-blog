---
layout: post
title: "`std::try_cast` and `(const&&)=delete`"
date: 2024-07-03 00:01:00 +0000
tags:
  exception-handling
  library-design
  smart-pointers
  st-louis-2024
---

[P2927 "Inspecting `exception_ptr`,"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2927r2.html)
proposes a facility (formerly known as `std::try_cast`) with this signature:

    template<class T>
    const T* exception_ptr_cast(const exception_ptr&) noexcept;

You might use it in a slight modification of
[Nicolas Guillemot's example from 2013](https://cppsecrets.blogspot.com/2013/12/using-lippincott-function-for.html)
([Godbolt](https://godbolt.org/z/178ThW8Ms)):

    FooResult lippincott(std::exception_ptr eptr) {
      assert(eptr != nullptr);
      if (auto *ex = std::exception_ptr_cast<MyException1>(eptr)) {
        return FOO_ERROR1;
      } else if (auto *ex = std::exception_ptr_cast<MyException2>(eptr)) {
        return FOO_ERROR2;
      } else {
        if (auto *ex = std::exception_ptr_cast<std::exception>(eptr)) {
          log::warning("%s", ex->what());
        }
        return FOO_UNKNOWN;
      }
    }

    try {
      foo::DoThing();
    } catch (...) {
      return lippincott(std::current_exception());
    }

> Notice the grotesque redundancy of the name `exception_ptr_cast`. In an ideal world you
> should name your functions primarily for what they _do_, not for what _argument types they take_.

Someone pointed out that it's dangerous to write

    if (auto *ex = std::exception_ptr_cast<std::exception>(std::current_exception())) {
      std::cout << ex->what() << "\n";
    }

because `std::current_exception` is permitted to copy the in-flight exception to storage that lives
only as long as the `exception_ptr`'s reference count remains above zero. MSVC actually does
something along these lines; see ["MSVC can't handle move-only exception types"](/blog/2019/05/11/msvc-what-are-you-doing/) (2019-05-11).
So if you write the line above, it will compile everywhere, work fine on Itanium-ABI platforms,
but cause a use-after-free on Windows.

"Okay, why don't we just make it ill-formed to call `exception_ptr_cast` with an rvalue
`exception_ptr`? Just `=delete` the overload that preferentially binds to rvalues.
Then you _have_ to store the thing in a variable, so you know the resulting `exception*` can't dangle. Like this:"

    template<class T> const T* exception_ptr_cast(const exception_ptr&) noexcept;
    template<class> void exception_ptr_cast(const exception_ptr&&) = delete;

<b>This, as all readers of my blog know, is wrong!</b> [Value category is not lifetime.](/blog/2019/03/11/value-category-is-not-lifetime/)
We certainly want to continue being able to write ([Godbolt](https://godbolt.org/z/d3x7v41qz)):

    if (Result r = DoThing(); r.has_error())
      if (auto *ex = std::exception_ptr_cast<std::exception>(r.error()))
        std::cout << ex->what() << "\n";

...even when `r.error()` returns by value, so that `r.error()` is a prvalue expression.
And vice versa, even if we were to `=delete` the rvalue overload as shown above,
that wouldn't stop someone from writing ([Godbolt](https://godbolt.org/z/7TP1Tqjo4)):

    auto *ex = std::exception_ptr_cast<std::exception>(DoThing().error());
      // dangles when r.error() returns by reference


## When is deleting `const&&` overloads desirable?

Short answer, I believe it's _never_ desirable. Because value category is not lifetime.

However, even if you believe that value category _is_ lifetime, there's still a major difference between
the places that the STL uses `(const&&)=delete` today, and the scenario with `exception_ptr_cast` sketched above.
The eleven places where `(const&&)=delete` appears today are:

    template<class T>
      const T* addressof(const T&&) = delete;
    template<class T>
      void as_const(const T&&) = delete;
    template<class T> void ref(const T&&) = delete;
    template<class T> void cref(const T&&) = delete;

and in [`ref_view`](https://eel.is/c++draft/range.ref.view#3), and in
[`reference_wrapper`](https://eel.is/c++draft/refwrap.const)'s single-argument constructor
(for parity with `std::cref`), and lastly, in the five constructors that construct
[`regex_iterator` or `regex_token_iterator`](https://eel.is/c++draft/re.iter) from `const regex&&`.

In all of these cases, the function being deleted is a function that would return you a pointer or
reference to _the argument itself_, which we know is syntactically an rvalue expression and thus is being
explicitly signaled by the programmer as "I'm done with this object" (regardless of how long it'll actually
live, relative to the useful lifetime of the returned pointer or reference).

In the `exception_ptr_cast` case, the function returns you a pointer, not to the _argument_ object,
but to _the thing pointed to by_ the argument object: not `eptr` but `*eptr`. Semantically, this is
the same operation as `shared_ptr::operator*`. Consider the potential for dangling here:

    int& r = *std::make_shared<int>(42);
    std::cout << r;
      // r is dangling!
    int *p = std::make_shared<int>(42).get();
    std::cout << *p;
      // p is dangling!

Yet we don't mark `shared_ptr::operator*() const&&` or `shared_ptr::get() const&&` as deleted;
because we realize that it's actually quite useful to be able to pass around `shared_ptr`s by value.
Even if value category _were_ lifetime, the lifetime of a single `shared_ptr` is only weakly correlated
with the lifetime of the object it points to.

Just for fun, I `=delete`'d the rvalue overloads of `get`, `*`, and `->` for both smart pointers
and recompiled Clang to see what broke. The surprising answer: A lot of things!

* [This use](https://github.com/llvm/llvm-project/blob/b00c8b9/clang/lib/CrossTU/CrossTranslationUnit.cpp#L570) of `CI.getPCHContainerOperations()`, exactly like the `r.error()` example above
* [This use](https://github.com/llvm/llvm-project/blob/b00c8b9/llvm/lib/IR/Metadata.cpp#L703) of `Context.takeReplaceableUses()`
* [This use](https://github.com/llvm/llvm-project/blob/b00c8b9/llvm/lib/IR/PassTimingInfo.cpp#L118) of `CreateInfoOutputFile`, which returns a `unique_ptr` that is dereferenced, used, and discarded
* [These uses](https://github.com/llvm/llvm-project/blob/b00c8b9/llvm/lib/LTO/ThinLTOCodeGenerator.cpp#L927) of `TMBuilder.create`, ditto
* [Almost every use](https://github.com/llvm/llvm-project/blob/b00c8b9/clang/utils/TableGen/ClangAttrEmitter.cpp#L3347) of `createArgument`, ditto (honestly a lot of these look like null dereferences waiting to happen)
* [This questionable use](https://github.com/llvm/llvm-project/blob/b00c8b9/clang/lib/StaticAnalyzer/Checkers/StdLibraryFunctionsChecker.cpp#L2119) of `NotNull`
* [This redundant `std::move`](https://github.com/llvm/llvm-project/blob/b00c8b9/llvm/lib/TableGen/Main.cpp#L163) probably indicates a bug of some kind
* [Here](https://github.com/llvm/llvm-project/blob/b00c8b9/clang/lib/Frontend/FrontendAction.cpp#L1112), an actual (though very minor) bug: `get` should have been `release`

In fact, LLVM contains at least [one instance](https://github.com/llvm/llvm-project/blob/b00c8b9/llvm/lib/Transforms/Utils/PredicateInfo.cpp#L944)
of the following pattern:

    std::make_unique<PredicateInfo>(F, DT, AC)->verifyPredicateInfo();

> Why not simply `PredicateInfo(F, DT, AC).verifyPredicateInfo()`? Maybe a `PredicateInfo` is
> extremely large, such that the latter would blow their stack. I don't think that's actually the case,
> but it's a plausible reason one might heap-allocate a temporary object like this.

In our `exception_ptr_cast` scenario, that would be like:

    std::cout << std::exception_ptr_cast<std::exception>(std::current_exception())->what();
      // no risk of dangling here

In conclusion: Deleting rvalue overloads is a bad way to deal with dangling bugs, because value category is not lifetime.
But even where the STL already uses `=delete`, it always uses it to address dangling references to _the argument itself_,
never to anything more abstractly "managed" or reference-counted by the argument. We want to preserve the ability to pass
and return `exception_ptr`s by value, and so we mustn't make prvalue `exception_ptr`s behave any differently from lvalue
`exception_ptr`s.
