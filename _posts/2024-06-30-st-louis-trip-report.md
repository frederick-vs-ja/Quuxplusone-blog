---
layout: post
title: "How my papers did at St Louis"
date: 2024-06-30 00:01:00 +0000
tags:
  language-design
  proposal
  relocatability
  triviality
  st-louis-2024
  stl-classic
  wg21
---

This past week I was at the WG21 meeting in St Louis, Missouri. My lovely wife and I spent a
fair bit of time exploring the city in vacation mode: [City Museum](https://citymuseum.org/plan-your-visit/things-to-find/)
(fantastic); the penguin house at the St Louis Zoo (fantastic);
cocktails, carpaccio, and hot chicken at [Blood & Sand](https://www.bloodandsandstl.com/) (fantastic).
But in between, there were some movements of papers.

## P1144R11 `is_trivially_relocatable`

No movement on [P1144](https://open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1144r11.html);
EWGI continues not to schedule it for discussion and not to forward it to EWG either.
But there was a big Friday discussion of the competing direction
[P2786](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2786r6.pdf),
informed by the recent papers
[P3233 "Issues with P2786"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3233r0.html) (Giuseppe D'Angelo) and
[P3236 "Please reject P2786 and adopt P1144"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3236r1.html) (several authors).
The result of this discussion was a strong vote (21–15–3–6–5) to "un-forward" P2786 and bring it back to EWG for further discussion
and revision. My utmost thanks to all the authors involved, especially the twelve signatories of P3236 (two of whom
managed to attend the meeting) and especially Giuseppe D'Angelo!

Recall the three main points of difference between P1144 and P2786:

<b>1.</b> Does the `is_trivially_relocatable` trait have “holistic”/“umbrella” semantics, tying directly into the Rule of Five
and relating an object's value semantics to its object representation, à la `is_trivially_copyable`?
Or does it talk specifically about the operation "move-construct and destroy," i.e., can the operation never be
used without messing with object lifetimes; and does the trait refer specifically to that operation, à la `is_trivially_copy_assignable`?

> Another way to put that is: There's a class property P1 "can be move-destroyed trivially" and a class property P2
> "can be move-destroyed trivially *and* swapped trivially." No codebase ever has names for both P1 and P2; the majority
> of third-party codebases have a trait `is_trivially_relocatable` modeling P2, and don't care about P1. A minority of codebases
> ([see the survey](/blog/2024/06/15/who-uses-trivial-relocation/); perhaps only [OE-Lib](/blog/2024/06/15/who-uses-trivial-relocation/#oe-lib))
> have a trait `is_trivially_relocatable` modeling P1, and don't care about P2.
> P1144 proposes that the STL should use the existing name `std::is_trivially_relocatable` for P2, and not care about P1.
> P2786 proposed that the STL should reassign the name `std::is_trivially_relocatable` to P1 instead, and someday come up
> with a new name, such as `is_trivially_swappable`, for P2 (with the corresponding contextual keyword).

<b>2.</b> Is the warrant syntax an ordinary attribute, e.g. `[[clang::trivially_relocatable]]`, appearing in the usual position for class attributes?
Or is it a new contextual keyword, appearing in trailing position like `final`?

<b>3.</b> Does the warrant have "sharp-knife" semantics, permitting the programmer to warrant the trivial relocatability of a class
that contains data members of unknown triviality, on pain of UB? Or does the warrant have
"dull-knife" semantics, being a no-op or ill-formed when applied wrongly, thus requiring application of the warrant
"virally downward" through all the class's bases and data members? (Note that the former, P1144's choice, formally classifies
all "buggy" misuse of the warrant as UB. The latter, P2786's choice, allows you to misuse the attribute to create behaviors
a human would certainly call "bugs," such as the segfault shown in [P3233 §8.3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3233r0.html#polymorphic),
while still seeming to classify such bugs as "well-defined behavior." I admit I have no good mental model of P2786's decisions in
this area.)

I expect that the St Louis discussion will result, at least, in a new revision of P2786 that brings P2786 into line with P1144
as regards point #1, and as regards polymorphic types. See
["Polymorphic types aren't trivially relocatable"](/blog/2023/06/24/polymorphic-types-arent-trivially-relocatable/) (2023-06-24).
I think point #3 will require EWG discussion. Point #2 will be the very last to fall.
But notice that points #2 and #3 relate only to the warrant mechanism, not to the meaning of the trait itself.

Once P2786 formally agrees with P1144 on point #1,
there will be no further reason for Clang maintainers to delay adopting [#84621](https://github.com/llvm/llvm-project/pull/84621),
which will give us our first mainstream compiler implementation of P1144/P2786 *sans* warrant syntax. The next step will be for
Clang to implement a warrant syntax that works in today's C++, which in fact is
[already in use](https://github.com/cppfastio/fast_io/blob/895f68fc9b23e8dc13918fe1ef0436eaebab79c1/include/fast_io_hosted/dll/posix.h#L110-L113)
by some libraries eagerly awaiting Clang's implementation.

In short, St Louis was a very good meeting for trivial relocatability. My utmost thanks again to Giuseppe and the P3236 folks!


## P2767R2 `flat_map`/`flat_set` omnibus

No movement; still in LWG and LEWG (depending on which part you're looking at).
But the first subsection's big editorial refactoring was merged before St Louis, which is great.


## P2848R0 `is_uniqued` (Enrico Mauro and myself)

Seen by the SG9 Ranges subgroup, and forwarded, with only a little questioning of the name.


## P2952R0 `auto& operator=(X&&) = default` (Matthew Taylor and myself)

Seen by EWGI in my absence and forwarded to EWG for next time.


## P2953R0 Forbid defaulting `operator=(X) &&`

No movement; still in EWGI.


## P3016R0 Inconsistencies in `begin`/`end` for `valarray` and `initializer_list`

No movement. As I understand it, P3016 was made ready to forward to LWG in Tokyo, but due to a snafu
wasn't actually electronic-polled before St Louis; so it will likely be seen by LWG in Wrocław.


## P3279 What "trivially fooable" should mean

[This paper](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3279r0.html) basically recaps the
horrors of ["Types that falsely advertise trivial copyability"](/blog/2024/05/15/false-advertising/) (2024-05-15),
and tries to find a way for the C++ Standard to express what all library vendors have assumed for decades
it actually meant. But P3279R0 is a big mess still lacking a clear proposed wording; so it hasn't yet been
discussed by any group, and that's fine. I'm hoping R1 will be clearer.


## Other papers

Peter Sommerlad's [P2968 "Make `std::ignore` a first-class object"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2968r2.html)
was voted into C++26 at this meeting! This small paper provides the Standard's blessing to code like

    std::ignore = f();

which, until now, was always technically UB.

Should you now go replace all your void casts, like `(void)effectful()`, with
`std::ignore = effectful()`? Of course not. Please god don't do that. But I'm very happy
that the Standard now formally defines the meaning of `std::ignore`, blessing common practice
and removing UB from the library.

----

Alan Talbot's [P0562 "Trailing Commas in Base-clauses and Ctor-initializers"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p0562r2.pdf)
proposes that, just like you can use trailing commas in array initializers and enum definitions,
you should be able to use them in _base-specifier-lists_ and _member-initializer-lists_ too.

See:

* ["Why you should enforce dangling commas for multiline statements"](https://medium.com/@nikgraf/why-you-should-enforce-dangling-commas-for-multiline-statements-d034c98e36f8) (Nik Graf, April 2016)
* ["On the virtues of the trailing comma"](https://devblogs.microsoft.com/oldnewthing/20240209-00/?p=109379) (Raymond Chen, February 2024)

P0562 almost made it to plenary, but was delayed at the eleventh hour by what looks like a serious grammatical ambiguity.
[Godbolt](https://godbolt.org/z/eocsnPT6j):

    struct X { X(int); };
    struct Y : X {
      Y(short) : A1<b1<1>(0)  {}
      Y(float) : A2<b2<1>(0), {}
  
      >(0){}

      template<bool> using A1 = X;
      template<int, int> using A2 = X;
      static constexpr int b1 = 2;
      template<int> using b2 = int;
      static constexpr int c = 0;
    };

That `>(0){}` cruft is actually still part of the `Y(float)` constructor definition! But how is the parser supposed to know it,
if we permit the comma after `(0)` to be a meaningless dangling comma? The parser hasn't seen the declaration of `A2` yet. Today,
apparently, this is handled by the assumption that a comma followed immediately by `{` *never* indicates the beginning of the
constructor's braced function body — that is, that trailing commas never appear.

Can this be fixed? I don't know.

Does any similar ambiguity affect the _base-specifier-list_ part of the proposal? I don't know.

----

Lénárd Szolnoki's [P2413 "Remove unsafe conversions of `unique_ptr`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2413r1.html)
arrived in LEWG with an R1 that was much more complicated than its R0, and was asked to come back less complicated.

[I implemented R0 in my Clang fork](https://github.com/Quuxplusone/llvm-project/commit/064121a7a346d19b98d8bdbbd0f8d9c248583b26#diff-b6df85f7d4e562cad9d12d1a00bad8dc7a8ce03d85639c287bf842b27308ebf8L61)
back in 2020. It simply removes the implicit conversion from `default_delete<Lion>` (which calls `delete` on a `Lion*`)
to `default_delete<Cat>` (which calls `delete` on a `Cat*`) except in the case that `Cat`'s destructor is virtual. Because if
calling `delete` on a `Cat*` that really (we assume) points to a `Lion` object, has undefined behavior except when `Cat`'s destructor is virtual.
This obscure change has the effect of also preventing conversion from `unique_ptr<Lion>` to `unique_ptr<Cat>`
(because the deleter type is no longer convertible). This has caught several real bugs in both LLVM/Clang and Chromium,
as well as in my own employer's codebase.

R1 extends R0 by also trying to catch situations like:

    auto p = std::unique_ptr<Cat>(new Lion);  // UB
    p.reset(new Lion);  // UB

This does catch real bugs; Lénárd showed me [LLVM #96980](https://github.com/llvm/llvm-project/issues/96980) as an example.
But it requires invasive changes to the API of `unique_ptr` itself, which is scary for a language that prides itself on backward
compatibility. In particular, R1 broke an idiom used at least nine times in Chromium:

    template<class T>
    struct raw_ptr {
      T *p_;
      operator T*() const { return p_; }
    };

    raw_ptr<Cat> r;
    auto p = std::unique_ptr<Cat>(r); // R1 made this ill-formed
    p.reset(r); // R1 made this ill-formed

So I hope we'll see P2413 return with the more conservative direction in the future.

----

Gonzalo Brito Gadeschi's [P0843 `inplace_vector`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p0843r12.html)
was adopted for C++26! I still think Pablo Halpern's proposal to add allocator support
(for the purpose of "passing through" non-trivial allocators to the elements à la `std::tuple`, and for the
benefit of fancy allocators like Boost.Interprocess) is a good idea, and I hope that he and/or I will bring
a proposal to add such support before C++26 ships. Maybe it'll be easier to sell as a small amendment on top
of the draft IS, than as a relatively invasive modification of Gonzalo's proposal.

----

Another important paper adopted for C++26 is [P2300 "`std::execution`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r9.html)
(9 authors), which is something to do with coroutines and/or multithreading and/or Sender/Receiver. You can see
some examples of P2300's API [in the paper's Examples section](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r9.html#example-echo-server).
It was pointed out that P2300 is too arcane for any ordinary programmer to comprehend, and as far as I can tell,
that's true; but (charitably) that's not _necessarily_ a reason to keep it out of the Standard, just a reason to
teach people not to use it. I don't expect ordinary mortals to use `std::linalg` or `std::rcu` either.

On the other hand, `std::linalg` and `std::rcu` passed their plenaries by unanimous consent (see
[N4970 motion 19](https://open-std.org/jtc1/sc22/wg21/docs/papers/2023/n4970.pdf),
[N4957 motions 6+7](https://open-std.org/jtc1/sc22/wg21/docs/papers/2023/n4957.pdf));
`std::execution` generated a lot of discussion and then passed with significant opposition
([N4985 motion 12](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/n4985.pdf), 57–27–20).
I think we'll see more activity in this area. If nothing else, we'll see some National Body comments.
