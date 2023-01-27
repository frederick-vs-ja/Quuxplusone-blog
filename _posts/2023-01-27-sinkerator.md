---
layout: post
title: "Simple C++20 input and output iterators"
date: 2023-01-27 00:01:00 +0000
tags:
  pearls
  type-erasure
  stl-classic
excerpt: |
  Sometimes I find myself rewriting a snippet often enough that I realize
  I should just blog it; this is one of those times.

  Here's a simple way to type-erase arbitrary sources and sinks
  into C++20 input iterators and output iterators.
---

Sometimes I find myself rewriting a snippet often enough that I realize
I should just blog it; this is one of those times.

Here's a simple way to type-erase arbitrary sources and sinks
into C++20 input iterators and output iterators. Notice how short
these implementations are, as of C++20. In particular,
we no longer need most of the "Big Five" `iterator_traits` typedefs
(unless we care about compatibility with `-std=c++17`): C++20's
`iterator_traits` is smarter than C++17's at inferring the missing
pieces.

## Output iterator

[Godbolt.](https://godbolt.org/z/94Tv6Wf1z)

    template<class T>
    class Sinkerator {
        struct Proxy {
            const Sinkerator *self_;
            void operator=(std::convertible_to<T> auto&& u) const {
                self_->f_(decltype(u)(u));
            }
        };
    public:
        using difference_type = int;
        explicit Sinkerator(std::function<void(T)> f) : f_(std::move(f)) {}
        Proxy operator*() const { return Proxy{this}; }
        Sinkerator& operator++() { return *this; }
        Sinkerator& operator++(int) { return *this; }
    private:
        std::function<void(T)> f_;
    };

    static_assert(std::output_iterator<Sinkerator<int>, short>);
    static_assert(!std::output_iterator<Sinkerator<int>, int*>);

Below, `it1` and `it2` have distinct types, but `jt1` and `jt2` have the
same type, `Sinkerator<int>`.

    auto it1 = std::ostream_iterator<int>(std::cout);
    auto it2 = std::back_inserter(v);

    auto jt1 = Sinkerator<int>([](int i) { std::cout << i; });
    auto jt2 = Sinkerator<int>([&](int i) { v.push_back(i); });

## Unidiomatic input iterator

Here's our first attempt ([Godbolt](https://godbolt.org/z/T3cb8PGGz));
this does work, but has some caveats discussed below.

    template<class T>
    class Sourcerator {
    public:
        using value_type = std::remove_cvref_t<T>;
        using difference_type = int;
        explicit Sourcerator(std::function<T()> f) : f_(std::move(f)) {}
        T operator*() const { return f_(); }
        Sourcerator& operator++() { return *this; }
        Sourcerator& operator++(int) { return *this; }
        bool operator==(const Sourcerator&) const; // unused
    private:
        std::function<T()> f_;
    };

    static_assert(std::input_iterator<Sourcerator<int>>);
    static_assert(std::is_same_v<std::iterator_traits<Sourcerator<int>>::iterator_category, std::input_iterator_tag>);

Below, `it1` and `it2` have distinct types, but `jt1` and `jt2` have the
same type, `Sourcerator<int>`.

    auto it1 = std::istream_iterator<int>(std::cin);
    auto it2 = std::views::iota(0).begin();

    auto jt1 = Sourcerator<int>([](){ int i; std::cin >> i; return i; });
    auto jt2 = Sourcerator<int>([i=0]() mutable { return i++; });

But input iterators are messier than output iterators: this `Sourcerator` type has no way to signal
"end of input." Most use-cases will need that. Also, evaluating `*it; *it; *it;` over and over
consumes data from the source; technically, that violates the semantic requirements of the
`std::input_iterator` concept, and it subtly differs from the behavior of
`std::istream_iterator` and `std::views::iota`.

## Idiomatic input iterator

A more idiomatic (if less performant) version goes like this ([Godbolt](https://godbolt.org/z/fxM5h39jb)).
C++20's `iterator_traits` assumes that any default-constructible iterator is at least a forward iterator, so
in this version (because of its default constructor) we must hard-code
its `iterator_category` to `input_iterator_tag`.

    template<class T>
    class Sourcerator {
    public:
        using value_type = T;
        using difference_type = int;
        using iterator_category = std::input_iterator_tag;
        explicit Sourcerator() = default;
        explicit Sourcerator(std::function<std::optional<T>()> f) : f_(std::move(f)), t_(f_()) {}
        const T& operator*() const { return *t_; }
        Sourcerator& operator++() { t_ = f_(); return *this; }
        Sourcerator operator++(int) { auto copy = *this; ++*this; return copy; }
        bool operator==(const Sourcerator& rhs) const { return !t_.has_value() && !rhs.t_.has_value(); }
    private:
        std::function<std::optional<T>()> f_;
        std::optional<T> t_;
    };

This one lets you, for example, pull all the data out of a `priority_queue` (and stop when there's
no more data to pull).

    auto pq = std::priority_queue<int>({}, {2,5,1,4,3});
    auto in = Sourcerator<int>([&]() {
        std::optional<int> result;
        if (!pq.empty()) {
            result = pq.top();
            pq.pop();
        }
        return result;
    });
    std::copy(in, Sourcerator<int>(), a);
    assert(pq.empty());
    assert(std::ranges::equal(a, std::initializer_list<int>{5,4,3,2,1}));

----

You might ask why the first line of that example snippet couldn't be

    std::priority_queue<int> pq = {2,5,1,4,3};

or why the last line couldn't be

    assert(std::ranges::equal(a, {5,4,3,2,1});

I'll tell you: I don't know.
