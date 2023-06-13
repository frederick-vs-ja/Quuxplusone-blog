---
layout: post
title: 'SFINAE special members or support incomplete types: Pick at most one'
date: 2020-02-05 00:01:00 +0000
tags:
  concepts
  implementation-divergence
  library-design
  metaprogramming
  standard-library-trivia
  templates
---

Here's something that comes up a lot on [the cpplang Slack](https://cppalliance.org/slack/).
Why is it that `std::vector<MoveOnlyType>` advertises copyability?

In practice, this often manifests as cryptic compiler errors in which the caret points somewhere completely
useless. [For example:](https://godbolt.org/z/fGYxTv)

    In file included from test.cpp:1:
    memory:1876:31: error: call to implicitly-deleted copy constructor of 'unique_ptr<int>'
                ::new((void*)__p) _Up(_VSTD::forward<_Args>(__args)...);
                                  ^   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    [...snip...]
    vector:1258:9: note: in instantiation of function template specialization
    'vector<unique_ptr<int>>::__construct_at_end<unique_ptr<int>*>' requested here
            __construct_at_end(__x.__begin_, __x.__end_, __n);
            ^
    test.cpp:6:8: note: in instantiation of member function
    'vector<unique_ptr<int>>::vector' requested here
    struct Widget {
           ^

Why are we trying to copy a `Widget`? Why does a `Widget` even think that it's copyable
in the first place, given that one of its members is an "obviously non-copyable" `vector<MoveOnlyType>`?
Well, it's because <b>`vector` always advertises copyability.</b>

    using MoveOnlyType = std::unique_ptr<int>;
    static_assert(
        std::is_move_constructible_v<MoveOnlyType> &&
        !std::is_copy_constructible_v<MoveOnlyType>
    );

    // Yet -- here's the surprising part! --
    using MoveOnlyVector = std::vector<MoveOnlyType>;
    static_assert(
        std::is_copy_constructible_v<MoveOnlyVector>
    );

The reason that `vector` always says it's copyable is that in C++, every container must choose between

- correctly SFINAEing its special members, and

- supporting incomplete types.

The tradeoff is physically inevitable. Let's see why.


## What does it mean to support incomplete types?

Consider the following user-defined type:

    struct RawNode {
        int data;
        RawNode *children_begin = nullptr;
        RawNode *children_end = nullptr;
        RawNode *children_endcapacity = nullptr;

        RawNode(const RawNode&) { ... }
        RawNode(RawNode&&) { ... }
        [...]
    };

    struct SafeNode {
        int data;
        std::vector<SafeNode> children;
    };

Observe that `SafeNode` is just `RawNode` with more safety built in. As far as the machine is concerned,
they have the same in-memory representation and behavior. But `SafeNode` delegates the resource
management of its `children` to the author of `std::vector`, which is good software-engineering practice
and permits us to follow the Rule of Zero. So, wearing our hat as the author of `std::vector`, we'd
really like to make sure that `SafeNode` is well-formed C++.

In contrast, think about how the following can't possibly work:

    struct InvalidNode {
        std::array<InvalidNode, 10> children;
    };

    array:143:9: error: field has incomplete type 'InvalidNode'
        _Tp __elems_[_Size];
            ^

A `Node` can't contain within itself an `array<Node, 10>`; but it _can_ contain within itself a `vector<Node>`.
This is essentially because `std::vector` _supports incomplete types,_ whereas `std::array` does not.


## What does it mean to correctly SFINAE your special members?

STL containers, practically by definition, are resource-management types; so they follow the Rule of Five
rather than the Rule of Zero. They must provide user-defined special members.

Let's write an extremely simple Rule-of-Five resource-management class.

    template<class T>
    class Manager {
        alignas(T) char data_[sizeof(T)];

        T& data() { return (T&)data_; }
        const T& data() const { return (const T&)data_; }
    public:
        Manager(Manager&& rhs) noexcept { ::new (data_) T(std::move(rhs.data())); }
        Manager(const Manager& rhs) { ::new (data_) T(rhs.data()); }
        Manager& operator=(Manager&& rhs) { data() = std::move(rhs.data()); return *this; }
        Manager& operator=(const Manager& rhs) { data() = rhs.data(); return *this; }
        ~Manager() { data().~T(); }
    };

But hang on, we've just created an unconditionally copyable class! ([Godbolt.](https://godbolt.org/z/VEmEaB))

    using MoveOnlyType = std::unique_ptr<int>;
    static_assert(std::is_copy_constructible_v<Manager<MoveOnlyType>>);

If we want our class to behave like `std::optional` or `std::tuple` and be copyable iff
`T` is copyable, then we need to add some SFINAE constraints to our special members. In C++03 through
C++17, you'd do this with a bunch of base classes. I'll show the much shorter C++2a approach,
which is to use `requires`-clauses to _constrain_ our special members.

    template<class T>
    class Manager {
        T& data();
        const T& data() const;
    public:
        Manager(Manager&& rhs) noexcept
            requires std::is_move_constructible_v<T>
            { ::new (&data()) T(std::move(rhs.data())); }
        Manager(const Manager& rhs)
            requires std::is_copy_constructible_v<T>
            { ::new (&data()) T(rhs.data()); }
        Manager& operator=(Manager&& rhs)
            requires std::is_move_assignable_v<T>
            { data() = std::move(rhs.data()); return *this; }
        Manager& operator=(const Manager& rhs)
            requires std::is_copy_assignable_v<T>
            { data() = rhs.data(); return *this; }
        ~Manager() { data().~T(); }
    };

    using MoveOnlyType = std::unique_ptr<int>;
    static_assert(
        std::is_move_constructible_v<Manager<MoveOnlyType>> &&
        !std::is_copy_constructible_v<Manager<MoveOnlyType>>
    );

But hang on again! We've just created a `Manager` that cannot be used with incomplete types.

    struct Incomplete;
    struct S {
        Manager<Incomplete> m;
    };

Is `S` copy-constructible? Well, its implicitly defaulted copy constructor is non-deleted iff
`Manager<Incomplete>` is copy-constructible, which is true iff `Incomplete` is copy-constructible...
and we don't know whether `Incomplete` is copy-constructible, because it's incomplete!

GCC and Clang/MSVC give different behavior here, by the way. Clang and MSVC will attempt to instantiate `S`'s defaulted
copy constructor as soon as they see the closing brace of `S`, which means they treat this code as a hard error.
GCC waits to see if the defaulted copy constructor is needed, which means you can even use variables of type `S`
as long as you don't use any of `S`'s constructors or assignment operators. (Using any constructor, even the default
constructor, would force the compiler to figure out which constructors were viable candidates for overload
resolution; which would mean figuring out whether the copy constructor exists.)

----

To put it another way: If you aim always to correctly SFINAE your special members, then you will have trouble
figuring out the "correct" answer in cases like this one.

    struct Node {
        std::vector<Node> children;
    };

Is `Node` copyable? Well, it's copyable iff its member `children` is copyable; and `children` is copyable
iff `Node` is copyable. So we have a logical loop with no clearly "correct" answer at all.


## Which STL containers support incomplete types and thus are "always copyable"?

It varies from vendor to vendor. Here is a table I compiled by looking at libstdc++, libc++, and MSVC on
[Godbolt](https://godbolt.org/z/vim_WB). This table lists only library types where copying a `LibraryType<T>`
fundamentally requires copying a `T`; so for example I don't list `shared_ptr<T>` or `shared_future<T>`.

"I" stands for "supports incomplete types `T`, and thus must be unconditionally copyable."<br>
"C" stands for "conditionally copyable, and thus type `T` must be complete."<br>
"U" stands for "unconditionally copyable, but also, fails to support incomplete types `T`" — that is,
types marked `U` combine the disadvantages of the other two kinds.

| Library type                         | libc++ | libstdc++ | MSVC |
|:------------------------------------:|:------:|:---------:|:----:|
| `pair<T, U>`                         | C      | C         | C    |
| `tuple<Ts...>`                       | C      | C         | C    |
| `variant<Ts...>`                     | C      | C         | C    |
| `optional<T>`                        | C      | C         | C    |
| `array<T, N>`                        | C      | C         | C    |
| `deque<T>`                           | U      | I         | U    |
| `forward_list<T>`                    | I      | I         | I    |
| `list<T>`                            | I      | I         | I    |
| `{multi,}map<T,T>`                   | I      | I         | I    |
| `{multi,}set<T>`                     | I      | I         | I    |
| `unordered_{multi,}map<K,T>`         | I      | U         | I    |
| `unordered_{multi,}set<T>`           | I      | U         | I    |
| `vector<T>`                          | I      | I         | I    |
| `istream_iterator<T>`                | C      | U         | C    |
| `valarray<T>`                        | I      | U         | I    |
