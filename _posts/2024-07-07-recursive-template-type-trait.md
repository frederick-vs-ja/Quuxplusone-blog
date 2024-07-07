---
layout: post
title: "A `priority_tag`-like pattern for type-based metaprogramming"
date: 2024-07-07 00:01:00 +0000
tags:
  metaprogramming
  templates
  type-traits
excerpt: |
  Via Enrico Mauro, an interesting variation on the `priority_tag` trick.
  Compare the following example to the `HasSomeKindOfSwap` example in
  ["`priority_tag` for ad-hoc tag dispatch"](/blog/2021/07/09/priority-tag/) (2021-07-09).

      template<class T, int N = 2, class = void>
      struct HasSomeKindOfValueType
        : HasSomeKindOfValueType<T, N-1> {};

      template<class T>
      struct HasSomeKindOfValueType<T, 2, std::void_t<typename T::value_type>>
        : std::true_type {};
---

Via Enrico Mauro, an interesting variation on the `priority_tag` trick.
Compare the following example to the `HasSomeKindOfSwap` example in
["`priority_tag` for ad-hoc tag dispatch"](/blog/2021/07/09/priority-tag/) (2021-07-09).

    template<class T, int N = 2, class = void>
    struct HasSomeKindOfValueType
      : HasSomeKindOfValueType<T, N-1> {};

    template<class T>
    struct HasSomeKindOfValueType<T, 2, std::void_t<typename T::value_type>>
      : std::true_type {};

    template<class T>
    struct HasSomeKindOfValueType<T, 1, std::void_t<typename T::element_type>>
      : std::true_type {};

    template<class T>
    struct HasSomeKindOfValueType<T, 0>
      : std::false_type {};

    static_assert(HasSomeKindOfValueType<std::vector<int>>::value);
    static_assert(HasSomeKindOfValueType<std::shared_ptr<int>>::value);
    static_assert(!HasSomeKindOfValueType<int>::value);

This snippet cleanly expresses the logic: "If `T::value_type` exists, return true; otherwise if `T::element_type` exists,
return true; otherwise return false." It combines two template-metaprogramming
tricks we've seen before:

* The "recursive template," where `X<2>` derives from (or calls) `X<1>`, which derives from (or calls) `X<0>`.

* The ["`Enable` parameter,"](/blog/2019/04/26/std-hash-should-take-another-template-parameter/) where you hang an
    extra `class Enable = void` parameter off the end of your trait precisely so that it can be used for SFINAE.

The `Enable` parameter isn't technically needed in C++20; we have `requires` for that, now.
The following C++20 expresses the same logic as the C++17 above.

    template<class T, int N = 2>
    struct HasSomeKindOfValueType
      : HasSomeKindOfValueType<T, N-1> {};

    template<class T> requires requires { typename T::value_type; }
    struct HasSomeKindOfValueType<T, 2>
      : std::true_type {};

    template<class T> requires requires { typename T::element_type; }
    struct HasSomeKindOfValueType<T, 1>
      : std::true_type {};

    template<class T>
    struct HasSomeKindOfValueType<T, 0>
      : std::false_type {};

---

See also:

* ["Why do we require `requires requires`?"](/blog/2019/01/15/requires-requires-is-like-noexcept-noexcept/) (2019-01-15)
