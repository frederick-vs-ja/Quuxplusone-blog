---
layout: post
title: "`static constexpr unsigned long` is C++'s \"lovely little old French whittling knife\""
date: 2021-04-03 00:01:00 +0000
tags:
  c++-learner-track
  c++-style
  linguistics
---

There's [a meme](https://www.bbc.com/culture/article/20160908-the-language-rules-we-know-but-dont-know-we-know)
going around (extracted from Mark Forsyth's 2013 book
[_The Elements of Eloquence_](https://amzn.to/2QZkZqR))
that postulates an unwritten rule of English:

> Adjectives in English absolutely have to be in this order:
> opinion-size-age-shape-colour-origin-material-purpose Noun.
> So you can have a lovely little old rectangular green French
> silver whittling knife. But if you mess with that word order
> in the slightest you'll sound like a maniac.

The existence of compound modifiers complicates matters: personally
I would interpret "French silver knife" as meaning a knife composed of
French silver, not a silver knife that _was_ French. I've also never
heard of a rectangular knife; but I think part of Forsyth's point is
that the _word order_ is largely invariant regardless of whether the
overall image makes sense. Consider Noam Chomsky's nonsensical, but
grammatical, "colorless green ideas" — would "green colorless ideas"
feel just a bit less grammatical?

Over on [Language Log](https://languagelog.ldc.upenn.edu/nll/?p=27890)
Jason A. Quest observes that this rule ranks adjectives

> in terms of how intrinsic and fundamental they are to the object,
> from least to most. "Whittling knife" is essentially a compound noun [...]
> What it's made of and where it comes from are unchangeable characteristics
> of it, so those have to be close. Working back from there the adjectives
> become more superficially descriptive and possibly changeable, and then
> finally subjective. You could even extend the rule to say that ownership
> ("Jimmy's") goes before all else, which is a fully extrinsic adjective.

(Commenter Martha points out that an "adjective" like "Jimmy's" is in fact a
_[possessive determiner](https://en.wikipedia.org/wiki/Possessive_determiner)_,
and indeed it is a _written_ rule that possessive determiners should precede the
entire noun phrase no matter what.)

(Commenter Hector adds that we see the most-fundamental-binds-tightest
rule also in insults to masculinity: if you want to insult a guy, you call
him an "ugly little _whatever_," because "little" is the most salient insult;
but if he's tall then you reverse the order to "you big ugly _whatever_,"
because "big" isn't particularly insulting at all.)


## I thought this blog was about C++

C++, like English, gives us plenty of freedom to rearrange our adjectives
without breaking the _written_ grammar of the language. But in C++, as in English,
the rule of "most important descriptor binds tightest" still applies. Consider
these two declarations ([Godbolt](https://godbolt.org/z/n84cWrxar)):

    static inline constexpr unsigned long long int x = 42;
    long int long inline unsigned constexpr static y = 42;

C++ considers these declarations equally grammatical. But which one would you rather see
in a code review? I hope it's the first one!

C++'s rule for things-that-come-before-the-identifier is pretty amorphous.
[[dcl.spec.general] ](http://eel.is/c++draft/dcl.spec.general#1) simply lists
all the possible _decl-specifier_ keywords — `friend`, `virtual`, `explicit`,
`inline`, `constexpr`, `typedef`... — and lets you put them together in any order
you choose, just as English allows you to put together adjectives in any order
you choose. This lax formal grammar was inherited from C89:
C allowed `long unsigned const x`, so it remained valid in C++.

But when you actually write C++ code, I'd appreciate it if you'd channel
Mark Forsyth and _think_ of decl-specifiers as absolutely having to be in this order:
attributes-friendness-storage-constness-virtualness-explicitness-signedness-length-type Identifier.

<b>Signedness-length-type:</b> Write `unsigned long`, not `long unsigned`.
Observe that `unsigned long int` is intimately related to `long int`
in a way that `unsigned int` and `unsigned long int` are not:
there are [places](https://eel.is/c++draft/expr#basic.lval-11.2) where
signedness is ignored by the language, and there's a
[`make_signed` type trait](https://en.cppreference.com/w/cpp/types/make_signed)
but no "`make_long`" trait. So longness is in a real sense more "fundamental,"
more "important," than signedness.

As for the "type" part: I write `unsigned`, not `unsigned int`; and `unsigned long`, not
`unsigned long int`. But if I ever had to use C++'s long double type,
you can bet I'd write `long double` and not `double long`!

<b>Explicitness:</b> All of your constructors and conversion operators should
always be `explicit`, except for the ones you intend to call implicitly (i.e.,
your copy and move constructors, plus rare special cases like `string(const char *)`).
So my mental model of `explicit` is as a keyword that says nothing more than
"Look out, reader, here comes a constructor or conversion operator!" It's essentially
part of the name of the member function, and belongs right next to the identifier.

    constexpr explicit operator bool() const;
              //~~~~~~~~~~~~~~~~~~~~
              // This whole thing is the "name"

    explicit constexpr operator bool() const;
    //~~~~~~           ~~~~~~~~~~~~~
              // Splitting up the "name" obfuscates the code

<b>Virtualness:</b> Observe that `virtual` affects the semantics of a function
a lot more than `constexpr` does. It also conveys more information in the strictly
information-theoretic sense: the average class (that uses `virtual` at all) tends
to have _some_ virtuals and _some_ non-virtuals, whereas the average class tends
to have pretty much _all_ of its methods be constexpr or else _none_ of them be
constexpr. It's pretty common to take an existing class like

    struct S {
        S(int);
        virtual int do_foo();
        bool bar() const;
    };

and slap a big streak of `constexpr` paint down the left margin:

    struct S {
        constexpr S(int);
        constexpr virtual int do_foo();  // since C++20
        constexpr bool bar() const;
    };

_Nobody_ does that with `virtual` (and if they do, they need to stop).

<b>Constness:</b> C++20 introduces a lot more constness-related keywords besides
`constexpr`, but at least so far they are all [mutually exclusive](http://eel.is/c++draft/dcl.spec.general#2.sentence-2):
any given declaration can be `constexpr` OR `consteval` OR `constinit` but never more than one at a time.

C++11 replaced the "static const" idiom with the "static constexpr" idiom:

    static const bool a = ...;
    static constexpr bool b = ...;

and I hope nobody's out there using the "constexpr static" ordering!

Notice that [west const style](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rl-const)
falls naturally out of this "important things bind tighter" rule. The style shown above
de-emphasizes the constness of `bool b`, it's true; but that's better than a style that
de-emphasizes the boolness of `const b`!

Now, `constinit` is a little weird. It fills the same grammatical niche as `const` and `constexpr`
(and now `consteval`), so you'd think we should also write

    static constinit bool c = ...;

But actually, I could totally see putting `constinit` before the storage class:

    constinit static bool d = ...;

The more salient thing about `d` is that it's _not_ a const variable; you can modify
it at runtime. Subordinate to that, incidentally, it happens to be initialized with
a compile-time constant value. Just as with `constexpr`, I could see someone slapping
a big streak of `constinit`-colored paint down the left margin of their global variable
definitions, regardless of storage class. So maybe `constinit` is better off further left,
at least in some cases. (Ralph Waldo Emerson once said that constinitcy was the hobgoblin
of little minds. I agree — at least in some cases.)

<b>Storage class:</b> The C++ storage-class specifiers are `extern`, `static`, `thread_local`,
and `mutable`; to which we can add `inline` for historical reasons. As with
`static const`, there's a long C tradition of writing `static inline` (not `inline static`).
And, re our general rule of "more important adjectives bind tighter," we observe that
the inline-ness of a function in practice matters even less (to the caller) than its
constexpr-ness. So:

    static inline constexpr int f() { ... }

Indeed, between `constexpr inline` and `inline constexpr`, the C++ Standard itself
[prefers](http://eel.is/c++draft/meta.type.synop) `inline constexpr`.

> I rarely see `static inline` together anymore in C++.
> Static free functions don't benefit from `inline`. For static member functions,
> I recommend putting `inline` on the _definition,_ never on the declaration
> (this reduces clutter in the class body, and also reduces churn when you
> move a member function from the .h file into the .cpp file or vice versa);
> whereas the `static` keyword goes on the declaration inside the class body,
> and is grammatically forbidden to go on the definition.
>
> But in C++17 we got `static inline` member variables, which can be initialized
> directly in the class body even if they're non-const or non-scalar.
> This may cause the wild population of `static inline` to rebound slightly.

<b>Friendness:</b> To befriend an entity in C++, you copy-and-paste
its declaration directly into the body of your class, and then you slap a `friend`
keyword on the front.

    constexpr unsigned int f();

    class Secretive {
        friend constexpr unsigned int f();
    };

Slapping the `friend` keyword into the middle of the declaration would just
be weird.

Unfortunately, to befriend a template you _must_ put the _template-head_
(if any) in front of the `friend` keyword:

    template<class T> int f(T);

    class Secretive {
        friend template<class T> int f(T);  // invalid, sadly
        template<class T> friend int f(T);  // OK
        template<class T> int friend f(T);  // technically valid, but please don't

        friend int f(auto);  // valid since C++20 (but probably don't?)
    };

Finally, <b>attributes:</b> C++11 attributes are not
_decl-specifiers_, and therefore the formal grammar requires that they come either
first-of-all or last-of-all:

    [[nodiscard]] constexpr int f() { return 1; }  // OK
    constexpr int f [[nodiscard]]() { return 1; }  // valid, but please don't

If you accidentally place an attribute in the middle of your _decl-specifier-seq_,
the compiler will give you a diagnostic if you're lucky, and then proceed to ignore it.
(Clang and ICC error; GCC warns; MSVC doesn't even warn. Ironically, if you put the
attribute in the right place, ICC will ignore it anyway. [Godbolt](https://godbolt.org/z/Ehx1YYh6M).)

    constexpr [[nodiscard]] int f() { return 1; }  // invalid

    int main() { f(); }  // no diagnostic here because f is not a nodiscard function

By the way, `constinit` was [originally proposed as an attribute](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1143r0.md),
and probably should have remained one; it was [changed](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1143r1.md)
to a keyword only after discussion within WG21.


## What about the stuff at the _end_ of a declaration?

C++'s very lax _decl-specifier-seq_ was inherited from C.
But the idea of putting stuff after the function parameter list was a C++ invention,
so C++ has been able to enforce stricter formal rules about the ordering of those
clauses. The relevant grammar is given in the standard
under [_parameters-and-qualifiers_](http://eel.is/c++draft/dcl.decl.general#nt:parameters-and-qualifiers)
and [_member-declarator_](http://eel.is/c++draft/class.mem.general#nt:member-declarator),
and it goes pretty rigidly like this:

* Function name and parameter list
* Cv-qualifier
* Ref-qualifier
* Exception specification (`noexcept`)
* Trailing return type
* `override`, `final`, or `requires`
* Body, `=0`, `=default`, or `=delete`

For example:

    auto f() const & noexcept -> int override = 0;

    int g() && noexcept(false) requires true = delete;

Notice that as a general rule, we work toward completing the function's _type_ first,
because the function type is the most important thing.
(Noexceptness became part of the function type in C++17.) Only once we've completed the
function type do we move on to lesser properties such as its constraints and whether
it is a virtual overrider. This general rule explains the awkward not-so-trailing placement
of the trailing return type:

    // OK, looks great
    auto f() noexcept
        -> std::conditional_t<B, X, Y>;

    // Awkward
    auto g()
        -> std::conditional_t<B, X, Y>
        override;

    // Awkward
    auto h()
        -> std::conditional_t<B, X, Y>
        requires (N < 7);

Virtual functions can't be constrained, so you'll never see the
`override` or `final` keyword coexist with a `requires`-clause. Although it's
_possible_ to write either `final override` or `override final`, you shouldn't;
just write `final` alone. (Vaguely related:
["A hole in Clang’s `-Wsuggest-override`"](/blog/2021/02/19/virtual-final-silences-override-warning/) (2021-02-19).)

> C++'s syntax expands, gaslike, to fill the available space:
> `int h() requires true && override` is also a valid declaration,
> if a `constexpr bool override` is visible in the current scope.
> Fortunately, `int h() requires true && noexcept(false)` is
> invalid: the constraints in a _requires-clause_
> must all be _primary-expressions_, and `noexcept(false)` is
> a _unary-expression_. See
> ["Why do we require `requires requires`?"](/blog/2019/01/15/requires-requires-is-like-noexcept-noexcept/)
> (2019-01-15).

----

Guidelines mentioned or alluded to in this post:

* Attributes-friendness-storage-constness-virtualness-explicitness-signedness-length-type Identifier.

* Make every constructor `explicit` (except for copy, move, and very special cases).

* Make every conversion operator `explicit`: especially `explicit operator bool() const`.

* For a virtual function, use _exactly one_ of `virtual`, `override`, or `final`.

* Indent trailing `requires`-clauses the same way you'd indent trailing return types.

* [West const.](/blog/2018/03/15/east-const-west-const/)
