---
layout: post
title: "What library types are `trivially_relocatable` in practice?"
date: 2019-02-20 00:01:00 +0000
tags:
  implementation-divergence
  kona-2019
  relocatability
---

In my libc++ fork implementing library support for
[P1144 "Object relocation in terms of move plus destroy,"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1144r2.html)
I have [a unit test](https://github.com/Quuxplusone/libcxx/blob/trivially-relocatable/test/libcxx/type_traits/is_trivially_relocatable.pass.cpp)
that tests `std::is_trivially_relocatable_v<X>` for every library-provided type `X`
that I can think of. Here is that same list of library-provided types, in convenient tabular format,
for each of the Big Three implementations (libc++, libstdc++, and MSVC). The results for libc++
are accurate. The results for libstdc++ and MSVC are my best guesses based on whatever homework
I've done up to this point. I will endeavor to fix bugs in this table as they are reported.

The rightmost column, "Lowest," shows the lowest common denominator: what we could mandate
in the Standard without causing any ABI breakage for vendors.

Key:
- `✓✓` means it's always trivially copyable (which implies trivially relocatable).
- `✓` means it's always trivially relocatable.
- `C` means it's a template that is _conditionally_ trivially relocatable.
- `No` means it's never trivially relocatable.
- `D` means it's never trivially relocatable in debug mode, but always trivially relocatable in release.
- `CD` means it's never trivially relocatable in debug mode, but conditionally trivially relocatable in release.
- `?` means I have only a tentative guess.

| Library type                         | libc++ | libstdc++ | MSVC | Lowest |
|:------------------------------------:|:------:|:---------:|:----:|:------:|
| `reference_wrapper<T>`               | ✓✓ | ✓✓ | ✓✓ |  ✓✓  |
| `initializer_list<T>`                | ✓✓ | ✓✓ | ✓✓ |  ✓✓  |
| `allocator<T>`                       | ✓✓ | ✓ | ✓✓ |  ✓  |
| `pmr::polymorphic_allocator<T>`      | ✓✓ | ✓✓ | ✓✓* |  ✓✓  |
| `default_delete<T>`                  | ✓✓ | ✓✓ | ✓✓ |  ✓✓  |
| `hash<T>`                            | ✓✓ | ✓✓ | ✓✓ |  ✓✓  |
| `less<T>`                            | ✓✓ | ✓✓ | ✓✓ |  ✓✓  |
| `less<void>`                         | ✓✓ | ✓✓ | ✓✓ |  ✓✓  |
| `unique_ptr<T>`                      | ✓ | ✓ | ✓ |  ✓  |
| `unique_ptr<T, D>`                   | C | C | C |    C  |
| `shared_ptr<T>`                      | ✓ | ✓ | ✓ |  ✓  |
| `weak_ptr<T>`                        | ✓ | ✓ | ✓ |  ✓  |
| `pair<T, U>`                         | C | C | C |    C  |
| `tuple<Ts...>`                       | C | C | C |    C  |
| `variant<Ts...>`                     | C | C | C |    C  |
| `optional<T>`                        | C | C | C |  C  |
| `any`                                | No | No | No |  No |
| `locale`                             | ✓ | ✓ | ✓ |  ✓  |
| `exception_ptr`                      | ✓ | ✓ | ✓ |  ✓  |
| `exception`                          | No | No | No | No |
| `error_category`                     | No | No | No | No |
| `error_code`                         | ✓✓ | ✓✓ | ✓✓ |  ✓✓ |
| `error_condition`                    | ✓✓ | ✓✓ | ✓✓ |  ✓✓ |
| `errc`                               | ✓✓ | ✓✓ | ✓✓ |  ✓✓ |
| `type_index`                         | ✓✓ | ✓✓ | ✓✓ |  ✓✓ |
| `array<T, N>`                        | C | C | C |    C  |
| `deque<T>`                           | ✓ | ✓ | ✓ |  ✓  |
| `deque<T,A>`                         | C | C | C |    C  |
| `forward_list<T>`                    | ✓ | ✓ | ✓ |  ✓  |
| `forward_list<T,A>`                  | C | C | C |    C  |
| `list<T>`                            | No | No | ✓ |  No |
| `list<T,A>`                          | No | No | C |  No |
| `{multi,}map<K,V,C,A>`               | No | No | C |  No |
| `{multi,}set<T,C,A>`                 | No | No | C |  No |
| `unordered_{multi,}map<K,V,H,C,A>`   | No | No | C? |  No |
| `unordered_{multi,}set<T,H,C,A>`     | No | No | C? |  No |
| `vector<T>`                          | D | ✓ | ✓ |  No |
| `vector<T,A>`                        | CD | C | C |  No |
| `stack<T,C>`                         | C | C | C |   C |
| `queue<T,C>`                         | C | C | C |   C |
| `priority_queue<T,C>`                | C | C | C |   C |
| `vector<T,A>::iterator`              | C | C | C |   C |
| `unordered_map<K,V,H,C,A>::iterator` | CD | ✓ | C | C |
| `insert_iterator<Ctr>`               | ✓✓ | ✓✓ | ✓✓ | ✓✓ |
| `istream_iterator<T>`                | C | C | C | C |
| `ostream_iterator<T>`                | ✓✓ | ✓ | ✓✓ | ✓ |
| `regex`                              | ✓ | ✓ | ✓ | ✓ |
| `basic_regex<C,T>`                   | C | C | C | C |
| `smatch`                             | D | ✓ | ✓ | No |
| `string`                             | D | No | ✓ | No |
| `basic_string<C,T,A>`                | CD | No | C | No |
| `bitset<N>`                          | ✓✓ | ✓✓ | ✓✓ | ✓✓  |
| `chrono::system_clock::duration`     | ✓✓ | ✓✓ | ✓✓ | ✓✓  |
| `chrono::system_clock::time_point`   | ✓✓ | ✓✓ | ✓✓ | ✓✓  |
| `unique_lock<T>`                     | ✓ | ✓ | ✓ | ✓  |
| `shared_lock<T>`                     | ✓ | ✓ | ✓ | ✓  |
| `thread`                             | ✓ | ✓ | ✓ | ✓  |
| `thread::id`                         | ✓✓ | ✓✓ | ✓✓ | ✓✓  |
| `promise<T>`                         | ✓ | ✓ | ✓ | ✓  |
| `future<T>`                          | ✓ | ✓ | ✓ | ✓  |
| `shared_future<T>`                   | ✓ | ✓ | ✓ | ✓  |
| `function<T>`                        | No | ✓ | No | No |
| `packaged_task<T>`                   | No | ✓ | No | No |
| `complex<float>`                     | ✓✓ | ✓✓ | ✓✓ | ✓✓  |
| `mt19937`                            | ✓✓ | ✓✓ | ✓✓ | ✓✓  |
| `normal_distribution<float>`         | ✓✓ | ✓✓ | ✓✓ | ✓✓  |
| `valarray<T>`                        | ✓ | ✓ | ✓ | ✓  |

The following types aren't relocatable at all (because they aren't move-constructible):
`atomic<T>`, `atomic_flag`, `condition_variable`, `condition_variable_any`,
`lock_guard<T>`, `mutex`, `seed_seq`, `random_device`.


## Notes applicable to both libc++ and libstdc++

- `list`, `set`, and `map` all contain "end nodes" stored inside themselves (so, there are pointers from the heap to the object itself).

- `any` can store non-trivially-relocatable types inside its SBO buffer.

- `packaged_task` contains a `function`.

- `smatch` contains a `vector`.

- `exception` and `error_category` have non-final virtual destructors.

- `regex` does not contain a `string`; it contains a `shared_ptr` to a DFA.

- `basic_regex` contains a member of type `Traits`, so `basic_regex` is trivially relocatable iff `Traits` is trivially relocatable.

- `regex_traits<char>` contains a member of type `locale`, so it's not trivially copyable.

- `exception_ptr` and `locale` are just gussied-up (copy-only) `shared_ptr`s.

- `type_index` is just a gussied-up raw pointer to a `type_info`.


## Notes on libc++

- "Debugging" for containers means `_LIBCPP_DEBUG_LEVEL >= 2` (which is the same thing as `_LIBCPP_DEBUG_MODE`).

- `function` contains a pointer to its own SBO buffer.

- `function` can store non-trivially-relocatable types inside its SBO buffer.

- `vector::iterator` and `unordered_map::iterator` hold (possibly fancy) `pointer`s, so they're trivially relocatable iff the allocator's `pointer` type is trivially relocatable.

- `vector<bool>::iterator` is not trivially copyable. (Making it so would be an ABI break.)

- `valarray` is basically a `vector` as far as its memory management is concerned. It stores two pointers.


## Notes on libstdc++

- `function` will store `T` inside its SBO buffer only if `__is_location_invariant<T>`. This special trait is true only for trivially copyable types and for a specific `packaged_task` helper type.

- `deque` allocates a (sort of) "sentinel node" on the heap even in its default constructor, so it is not nothrow-move-constructible; nonetheless it is (conditionally) trivially relocatable.

- `vector::iterator` holds a (possibly fancy) `pointer`, but [`unordered_map::iterator` holds a raw pointer](https://godbolt.org/z/cT0uDj); the latter is always trivially relocatable as far as I can tell.

- `allocator` is not trivially copyable. (Making it so would be an ABI break.)

- `ostream_iterator` is not trivially copyable. (Making it so would be an ABI break.)

- `valarray` is basically a `vector` as far as its memory management is concerned. It stores a pointer and a length.


## Notes on MSVC

- `list` allocates a "sentinel node" on the heap even in its default constructor, so it is not nothrow-move-constructible. However, as far as I can tell, it _is_ (conditionally) trivially relocatable.

- `set` (et al.) allocates a "head node" on the heap, which acts as the one-past-the-end sentinel node and also holds pointers
      to the root node, the leftmost node in the tree, and the rightmost node in the tree.

- `unordered_set` (et al.) seem to be trivially relocatable in practice, but I admit I don't understand the code.
      Empirical evidence is provided by [this test program](https://godbolt.org/z/d1rMYhe85).

- `any` will store `T` inside its SBO buffer only if `is_trivially_move_constructible_v<T>`; but it permits `!is_trivially_destructible_v<T>`. There is a fast path for trivially copyable types.

- `function` can store non-trivially-relocatable types inside its SBO buffer.

- `vector::iterator` and `unordered_map::iterator` hold (possibly fancy) `pointer`s, so they're trivially relocatable iff the allocator's `pointer` type is trivially relocatable.

- `pmr::polymorphic_allocator` (correctly) has `=delete`d assignment operators, and MSVC (incorrectly) [reports such types as non-trivially copyable](https://godbolt.org/z/xf42b0). I've recorded it as trivially copyable with an asterisk.

- `locale` and `exception_ptr` are just gussied-up (copy-only) `shared_ptr`s.

- `valarray` is basically a `vector` as far as its memory management is concerned. It stores a pointer and a length.

----

See also Howard Hinnant's ["Container Survey"](http://howardhinnant.github.io/container_summary.html) (June 2015),
which tabulates which containers' special member functions are `noexcept`.
