---
layout: post
title: "How do C++ using-directives work?"
date: 2020-12-21 00:01:00 +0000
tags:
  argument-dependent-lookup
  c++-learner-track
  name-lookup
  namespaces
---

Recall that in C++ there's a difference between a _using-declaration_
and a _using-directive_.

    using std::string;  // using-declaration
    using namespace std;  // using-directive


## Using-declarations

A using-declaration is semantically similar to a declaration;
it introduces into the current scope a new meaning for a name. If that
name already had one or more meanings from outer scopes, those existing
meanings are _hidden_, or _shadowed_, by the new declaration.
[Godbolt:](https://godbolt.org/z/Khx1cb)

    namespace AnimalUtils {
        int foo(Zoo::Animal);
    }
    namespace Outer {
        int foo(Zoo::Lion);
        namespace Inner {
            int foo(Zoo::Cat);  // declaration hides Outer::foo
            int test1() {
                return foo(Zoo::Lion());
            }
            int test2() {
                using AnimalUtils::foo;  // using-declaration hides Inner::foo
                return foo(Zoo::Lion()); // calls AnimalUtils::foo
            }
        }
    }

The declaration of `foo` in `Inner` _hides_, or _shadows_, the previous meaning that `foo` had had;
so when we look up the name `foo` inside `test1`, we find only `Inner::foo`.
We don't even consider `Outer::foo` as a candidate.

The using-declaration of `foo` in `test2` _hides_, or _shadows_, the previous meaning that `foo`
had had; so when we look up the name `foo` inside `test2`, we find only `AnimalUtils::foo`.
We don't even consider `Inner::foo` as a candidate.

(So how does [the `std::swap` two-step](/blog/2020/07/11/the-std-swap-two-step/) work?
Well, besides the normal unqualified lookup, we also do a _separate_ argument-dependent lookup,
and merge the two candidate sets. So in this particular case, overload resolution would also
consider any meanings of `foo` declared in namespace `Zoo`. But there is no such `foo` in
this example.)


## Using-directives

Using-directives are subtler than using-declarations. But the good news is that
[you should never use them!](https://stackoverflow.com/questions/1452721/why-is-using-namespace-std-considered-bad-practice)
Seriously, don't ever write `using namespace Whatever`; and then you won't have
any trouble with them. Just pretend they don't exist.

<b>But</b>, let's talk about them anyway.

Using-directives also introduce new meanings for names into scopes. But the surprising
thing is that they don't introduce those meanings into the _current_ scope! A using-directive
introduces its new meanings into the scope which is the
[lowest common ancestor](https://en.wikipedia.org/wiki/Lowest_common_ancestor)
of the current scope and the target namespace's own scope.

In a directed acyclic graph (DAG), the "lowest common ancestor" of two nodes _A_ and _B_
can be found by tracing a path from _A_ up to the root, and tracing a path from _B_ up to
the root — the lowest common ancestor is the place where the two paths first meet up.

Alternatively, start tracing a "fully qualified path" downward from the root; the
lowest common ancestor corresponds to the longest common prefix of these fully qualified
names. For example, consider this hierarchy of namespaces:

    namespace NA { }
    namespace NB {
        namespace NC {
            namespace N1 { }
            namespace N2 { }
        }
    }

The lowest common ancestor of `NB::NC::N1` and `NB::NC::N2` is `NB::NC`.
The lowest common ancestor of `NA` and `NB::NC::N2` is the root (the global namespace).

So, consider this snippet of C++ code ([Godbolt](https://godbolt.org/z/h41d9T)):

    namespace NA {
        int foo(Zoo::Lion);
    }
    namespace NB {
        int foo(Zoo::Lion);
        namespace NC {
            namespace N1 {
                int foo(Zoo::Cat);
            }
            namespace N2 {
                int test() {
                    using namespace N1;
                    using namespace NA;
                    return foo(Zoo::Lion());
                }
            }
        }
    }

You might reasonably expect that when `test` passes a `Lion` to `foo`,
it would result in a call to `NA::foo(Lion)`, because we "using-directive'd"
`namespace NA`. Or at least it might call `NB::foo(Lion)`, since that's
what would happen in the absence of any using-directives at all.
But in fact it calls `N1::foo(Cat)`!

As shown in the diagram below: `using namespace N1` causes a declaration of
`N1::foo(Cat)` to be injected into the least common ancestor of `N1` and `N2`,
which is `NC`. This declaration of the name `foo` _hides_ any and all meanings
of `foo` introduced in higher-up scopes, such as `NB::foo(Lion)`.

`using namespace NA` causes a declaration of `NA::foo(Lion)` to be injected
into the least common ancestor of `NA` and `N2`, which is the global namespace.
This injected declaration ends up being hidden by the declaration of `NB::foo(Lion)`,
which is in turn hidden by the declaration of `N1::foo(Cat)` which has been
injected into namespace `NC`.

![Diagram of what happens in that example](/blog/images/2020-12-21-using-directive.png)

By the way, even though the declaration of `N1::foo` has been injected into `NC`,
you can't actually refer to it as `NC::foo`. It has been injected only for the purposes
of _unqualified_ lookups — and only for the purposes of unqualified lookups that happen
within this scope. Nobody outside of the scope of `test` is going to "see" that
declaration of `foo` injected into namespace `NC`.


### A `std::swap` mis-step

Consider the following bad code ([Godbolt](https://godbolt.org/z/6aPT71)):

    namespace detail {
        struct Impl {};
        void swap(Impl&, Impl&);

        template<class T>
        void example(T& x, T& y) {
            using namespace std;
            swap(x, y);
        }
    }

It's bad for three reasons. Number one, it fails to use the hidden friend idiom
for `swap(Impl&, Impl&)`. Number two, it uses `using namespace std`. (Both of these are
red flags that should insta-fail any code review you do.)
Number three, its use of `using namespace` is wrong in terms of the "`std::swap` two-step."
(See ["What is the `std::swap` two-step?"](/blog/2020/07/11/the-std-swap-two-step/) (2020-07-11).)
In the two-step, we _want_ to bring the meaning of `swap` from namespace `std` into our current scope,
so that `swap(x, y)` will consider that meaning in addition to any meaning assigned
by ADL. But what we're doing here, instead, is to bring the meaning of `swap` (and every
other name in `std`) from namespace `std` into _the least common ancestor namespace_ of
`std` and `detail`; i.e., into the global namespace.

So when we try to instantiate `example<int>`, unqualified lookup looks for declarations of
`swap` in our current scope working outward, and it finds `detail::swap(Impl&, Impl&)` and stops
there. It never finds the templated `std::swap` declared in the global namespace. Oops!
That's why the _correct_ version of the `std::swap` two-step uses a using-declaration and
not a using-directive.


## Using-directives in namespace scope

You can also put a _using-directive_ at namespace scope, like this
([Godbolt](https://godbolt.org/z/9EdMMo)):

    namespace ND::N3 {
        int foo(Zoo::Cat);
    }

    namespace NE {
        using namespace ND::N3;
        int foo(Zoo::Lion);
    }

    namespace ND {
        namespace N4 {
            int test3() {
                using namespace NE;
                return foo(Zoo::Lion());
            }
        }
    }

Using-directives are "transitive"; since `NE` visibly contains a using-directive
for `ND::N3`, it's as if `test3` contains _both_ `using namespace NE` _and_ `using namespace ND::N3`.

In this contrived example, the names from `NE` are injected into the least common ancestor
of `ND::N4` and `NE` (i.e., the global namespace). The names from `ND::N3` are injected into the
least common ancestor of `ND::N4` and `ND::N3` (i.e., `ND`). Then, unqualified
lookup for `foo` scans outward from `ND::N4::test3`, stopping as soon as it finds the declaration of
`ND::N3::foo(Cat)` that has been injected into namespace `ND`. That declaration _hides_
the ostensibly better-matching `NE::foo(Lion)` that was injected into the global namespace.

Note that the "transitivity" of using-directives applies only to using-directives that are _directly_
within the using'ed namespace. If `test3` had said `using namespace NE::N5`, it would have
transitively picked up any using-directives from `NE::N5`, but not from `NE::N5`'s parent namespace `NE`
(and not from the global namespace).

And how does all this interact with [`inline namespace`](https://en.cppreference.com/w/cpp/language/namespace#Inline_namespaces)?
Well, let's save that for another day.

----

Some of these examples probably surprised you. I hope the explanations made sense anyway, in retrospect.
But is any of this information _useful_ to a good C++ programmer? I would have to say <b>no it is not.</b>
Good C++ programmers don't use using-directives — and I hope this blog post has convinced you
to join them!

> Don't use `using namespace`.

As for _why_ using-directives work in this bizarrely unintuitive way, I've asked that
question [over on Stack Overflow](https://stackoverflow.com/questions/65365681/why-does-cs-using-namespace-work-the-way-it-does).
So if you've got a good answer, that's a great place to type it up!
