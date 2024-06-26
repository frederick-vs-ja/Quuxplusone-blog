---
layout: post
title: "How the STL uses `explicit`"
date: 2024-06-25 00:01:00 +0000
tags:
  constructors
  initialization
  library-design
  st-louis-2024
  standard-library-trivia
---

One of the papers on the docket for this week's WG21 meeting in St Louis is
[P3116 "Policy for `explicit`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3116r0.html) (Zach Laine, 2024).
The idea of "policy," in this context, is that [LEWG](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#cwg-ewg-ewgi-lewg-lewgi-lwg)
wants to have something like a "style guide" for proposal-authors. If a proposal comes in with
`noexcept` in the wrong places, or `explicit`, or `[[nodiscard]]`, we want to be able to quickly
tell the author how it ought to be, without a lot of the same discussion
happening on every paper. Like a house style guide in newspaper-editing: if our newspaper uses
the Oxford comma, and you bring in an article without it, then we can just point to the style guide,
make the fix, and move on [see?], without a lot of repeated discussion of the pros and cons of the comma
except insofar as you can argue that it belongs in *this particular article* for a *really good reason*.

So the idea of P3116 is basically to lay out the current "house style" of the C++ Standard document
(specifically as it relates to the `explicit` keyword) — not to innovate or break new ground, but
just to describe what the library clauses currently do, so that we can keep consistency with that
house style going forward. (Is consistency a virtue? Usually. And specifically in C++,
which is built on "static polymorphism" with templates, it's supremely important that "like things
look alike." Every inconsistency is a potential compiler error deep inside someone's template code;
every surface defect will be taught as a pitfall.)

---

Regular readers of my blog will know my own guideline for the usage of `explicit` in industry codebases:

> All your constructors should be `explicit` by default.
> Non-`explicit` constructors are for special cases.

See ["Most C++ constructors should be `explicit`"](/blog/2023/04/08/most-ctors-should-be-explicit) (2023-04-08).

But what's good in green-field industry code isn't always possible in 40-year-old Standard Library specification!
Yes, it's bad, that e.g. the iterator-pair constructor of `vector` is non-explicit, so that
this ([Godbolt](https://godbolt.org/z/9qPYb5rWq)) is UB:

    std::vector<int> v = {"1", "2"}; // UB

But there are billions of lines of code relying for over a decade on awful constructs like

    const char s[] = "hello";
    std::vector<int> v = {&s[0], &s[5]};  // OK since C++11, albeit poor style

thus IMHO it's simply not possible for WG21 to wake up tomorrow and mark `std::vector`'s iterator-pair constructor `explicit`.
It'd break the world.

So, if the STL doesn't follow *my* guidelines for the use of `explicit`, what policies
*does* it follow?

## The STL's `explicit` house style as I understand it

Again, this is a *bad set of guidelines* for industry code. If you came here looking for what *you*
should do in *your* code, go read ["Most C++ constructors should be `explicit`"](/blog/2023/04/08/most-ctors-should-be-explicit) (2023-04-08) instead.
These guidelines are specifically for types aiming to get into the Standard Library.

These guidelines are a slight expansion and clarification of what's in P3116R0 already
(thanks to Zach for that!). At some point, I expect that LEWG will adopt a policy (based
on a future revision of P3116); then this blog post will be outdated and the source of
truth (as to the "real" style policy) will become WG21's own [SD-9 document](https://isocpp.org/std/standing-documents/sd-9-library-evolution-policies).

Without further ado, the style!

* <b>0.</b> Deviation from the following rules is permitted; but every deviation must be motivated and justified
    by a good rationale.
    <br><span class="smaller">_Rationale:_ Note that "I like it" or "my employer does it this way" are not good rationales.</span>

* <b>1.</b> In general, there should be no `operator X` conversion functions except for `operator bool`.
    <br><span class="smaller">_Rationale:_ Prefer named accessors. Examples of justified deviation include `basic_string::operator basic_string_view`
    and `explicit chrono::year::operator int`.</span>

* <b>2.</b> `operator bool` should always be `explicit`.
    <br><span class="smaller">_Rationale:_ An `explicit` conversion to `bool`
    suffices to make the type *contextually convertible* to `bool`, which suffices for e.g. `if (p)`, `(p && true)`, etc.
    For example, `unique_ptr::operator bool` and `optional::operator bool` are both `explicit`.
    An example of justified deviation is `bitset::reference::operator bool`, which is non-`explicit` in order to support the syntax `bool b = ref`.
    It may also justify deviation if the type `T` needs to satisfy *boolean-testable*, which requires implicit convertibility;
    but this is very rare.</span>

* <b>3.</b> The appropriate `explicit`-ness for a constructor depends primarily on its *arity*: the number of arguments
    it accepts. Below are separate rules for one-argument, zero-argument, and multi-argument constructors.
    If a single overload is of variable arity (e.g. because of defaulted function arguments or variadic
    parameter packs), such that the rules below do not all agree about the appropriate `explicit`-ness,
    then you must split up that constructor into multiple overloads so that you can apply the rules.
    <br><span class="smaller">_Rationale:_
    An example of non-deviation is `set(initializer_list, const C& = C(), const A& = A())`,
    which conforms to rule 4 when called with one argument and rule 8 when called with two or three arguments;
    both rules agree that it should be non-`explicit`.
    Another example is `filesystem::path(string_type&&, format = auto_format)`, which justifies
    its deviation from rule 4 and conforms to rule 8.
    Even when not strictly required by this rule, it can be beneficial to split a constructor into multiple overloads;
    for example, in order to mark a zero-argument signature `noexcept`, or to avoid unnecessary
    default-construction of temporaries.
    See [P1163R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1163r0.pdf) (Nevin Liber, 2018),
    a proposal to bring the entire Library into conformance with this guideline.</span>

* <b>4.</b> Initializer-list constructors ([[dcl.init.list]](https://eel.is/c++draft/dcl.init.list#def:initializer-list_constructor)),
    copy constructors, and move constructors should be non-`explicit`. Otherwise, every
    single-argument constructor should be `explicit`.
    <br><span class="smaller">_Rationale:_ This prevents implicit conversions between unrelated types from different domains.
    An example of a justified deviation is `string(const char*)`; another is `optional<T>(optional<U>&&)`,
    which is conditionally `explicit`. An example of deliberate non-deviation
    is `explicit string(const StringViewLike&)`, where the user might be surprised if
    an implicit conversion from `string_view` allocated memory.</span>

* <b>5.</b> If the type provides a non-`explicit` initializer-list constructor, then it must also provide a non-`explicit`
    default constructor.
    <br><span class="smaller">_Rationale:_ Initialization from `{}` uses the default constructor, never the initializer-list constructor.</span>

* <b>6.</b> Except as indicated in the next rule, every zero-argument constructor should be non-`explicit`.

* <b>7.</b> If the type is a "tag type," then its zero-argument constructor should be `explicit`.
    <br><span class="smaller">_Rationale:_ Often, a non-`explicit` constructor for a tag type would be ineffective.
    For example, `in_place_index_t<N>` carries information in the name of the type which
    cannot be deduced from a bare `{}`. Elsewhere, treating a bare `{}` as a tag might
    lead to new ambiguities. [TODO FIXME: [Here](https://godbolt.org/z/xKsaGd1ne) is an
    example of ambiguity; but where does this pattern ever occur in the real STL?]</span>

* <b>8.</b> Constructors of arity 2 or greater should be non-`explicit`.
    <br><span class="smaller">_Rationale:_ Existing generic code may
    expect to be able to construct objects from initializer-lists without explicitly naming their type.
    For example, `pair(piecewise_construct_t, tuple, tuple)` is non-`explicit`.
    For example, `chrono::nonexistent_local_time`'s two-argument constructor is non-`explicit`.
    An example of a justified deviation is `explicit optional(in_place_t, Args&&...)`, where
    the user might expect that `optional<T> o = {in_place, 1, 2}` would call `T(in_place, 1, 2)`
    but in fact the constructor selected is `explicit optional(in_place_t, Args&&...)` and
    so the code is ill-formed.</span>

* <b>9.</b> Deduction guides should never be `explicit`.
    <br><span class="smaller">_Rationale:_ The `explicit` keyword has no effect
    on a deduction guide. See [LWG3451](https://cplusplus.github.io/LWG/issue3451).</span>


## Deviations in the current Standard Library

> This list may be incomplete; you can help by adding to it.

Deviations from rule 1 inside `<chrono>` (which I assume are all justifiable) include:

    explicit day::operator unsigned() const
    explicit month::operator unsigned() const
    explicit year::operator int() const
    year_month_day::operator sys_days() const;
    explicit year_month_day::operator local_days() const;
    year_month_day_last::operator sys_days() const;
    explicit year_month_day_last::operator local_days() const;
    year_month_weekday::operator sys_days() const;
    explicit year_month_weekday::operator local_days() const;
    year_month_weekday_last::operator sys_days() const;
    explicit year_month_weekday_last::operator local_days() const;
    explicit hh_mm_ss<P>::operator precision() const;
    zoned_time<D,P>::operator sys_time<D>() const;
    explicit zoned_time<D,P>::operator local_time<D>() const;

Justifiable deviations from rule 1 elsewhere:

    weak_ordering::operator partial_ordering() const;
    strong_ordering::operator partial_ordering() const;
    strong_ordering::operator weak_ordering() const;
    coroutine_handle<T>::operator coroutine_handle<void>() const;
    basic_string<C,T,A>::operator basic_string_view<C,T>() const;
    reference_wrapper<T>::operator T&() const;
    atomic<T>::operator T() const;
    atomic<T>::operator T() const volatile;
    atomic_ref<T>::operator T() const;
    filesystem::path::operator string_type() const;
    ranges::in_fun_result<I,F>::operator in_fun_result<J,G>() const &;
    ranges::in_fun_result<I,F>::operator in_fun_result<J,G>() &&;
      // ...
    ranges::out_value_result<O,T>::operator out_value_result<P,U>() const &;
    ranges::out_value_result<O,T>::operator out_value_result<P,U>() &&;
    ranges::subrange::operator PairLike() const;

Justified deviations from rule 2:

    vector<bool>::reference::operator bool() const;
    bitset<N>::reference::operator bool() const;

Every single-argument *converting constructor* ([[class.conv.ctor]](https://eel.is/c++draft/class.conv.ctor#def:constructor,converting))
justifiably deviates from rule 4. These include:

    basic_string<C,T,A>(const C*, const A& = A());
    basic_string<C,T,A>(nullptr_t) = delete;
    coroutine_handle<T>(nullptr_t);
    allocator<T>(const allocator<U>&);
    pmr::polymorphic_allocator<T>(memory_resource*);
    pmr::polymorphic_allocator<T>(const polymorphic_allocator<U>&);
    scoped_allocator_adaptor<A,As...>(const scoped_allocator_adaptor<B,As...>&);
    scoped_allocator_adaptor<A,As...>(scoped_allocator_adaptor<B,As...>&&);
    shared_ptr<T>(nullptr_t);
    shared_ptr<T>(const shared_ptr<Y>&);
    shared_ptr<T>(shared_ptr<Y>&&);
    shared_ptr<T>(const weak_ptr<Y>&);
    shared_ptr<T>(unique_ptr<Y,E>&&);
    weak_ptr<T>(nullptr_t);
    weak_ptr<T>(const weak_ptr<Y>&);
    weak_ptr<T>(weak_ptr<Y>&&);
    weak_ptr<T>(const shared_ptr<Y>&);
    unique_ptr<T,D>(nullptr_t);
    unique_ptr<T,D>(unique_ptr<Y,E>&&);
    expected<T,E>(const expected<U,G>&);
    expected<T,E>(expected<U,G>&&);
    explicit(see below) expected<T>(U&&);
    optional(nullopt_t);
    explicit(see below) optional<T>(U&&);
    explicit(see below) optional<T>(const optional<U>&);
    explicit(see below) optional<T>(optional<U>&&);
    variant<Ts...>(T&&);
    function(nullptr_t);
    function(F&&);
    move_only_function(nullptr_t);
    move_only_function(F&&);
    copyable_function(nullptr_t);
    copyable_function(F&&);
    function_ref(F*);
    function_ref(F&&);
    bitset<N>(unsigned long long);
    reverse_iterator<T>(const reverse_iterator<U>&);
    move_iterator<T>(const move_iterator<U>&);
    move_sentinel<T>(const move_sentinel<U>&);
    explicit(see below) complex<T>(const complex<X>&);
    valarray(const slice_array<T>&);
    valarray(const gslice_array<T>&);
    valarray(const mask_array<T>&);
    valarray(const indirect_array<T>&);
    chrono::duration<R,P>(const duration<S,Q>&);
    chrono::time_point<C,D>(const time_point<C,E>&);
    chrono::weekday(const sys_days&);
    chrono::year_month_day(const year_month_day_last&);
    chrono::year_month_day(const sys_days&);
    chrono::year_month_weekday(const sys_days&);
    chrono::zoned_time<D,P>(const sys_time<D>&);
    chrono::zoned_time<D,P>(const zoned_time<F,P>&);
    error_code(ErrorCodeEnum);
    error_condition(ErrorConditionEnum);
    atomic<T>(T);
    shared_future<R>(future<R>&&);
    text_encoding(text_encoding::id);
    ranges::subrange(R&&);
    ranges::ref_view<R>(T&&);
    ranges::owning_view<R>(R&&);

`string_view` and `span` also have converting constructors that justifiably deviate from rule 4:

    basic_string_view<C,T>(const C*);
    basic_string_view<C,T>(nullptr_t) = delete;
    span<T,N>(T(&)[N]);
    span<T,N>(array<T,N>&);
    span<T,N>(const array<T,N>&);
    explicit(see below) span<T,N>(R&&);
    explicit(see below) span<T,N>(const span<U,P>&);

`span<T,N>` for `N != dynamic_extent` even justifiably deviates from rule 4's guidance that initializer-list constructors
should be non-`explicit`! This is so that `{1,2,3}` will implicitly convert to `span<const int>` but not to
`span<const int, 100>`; see [P2447](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2447r6.html).

    explicit(see below) span<T,N>(initializer_list);

`scoped_allocator_adaptor` deviates from rules 3+4, without obvious justification:

    scoped_allocator_adaptor<A,As...>(B&&, const As&...);

Among exception-hierarchy types, `system_error` deviates from rule 4;
`ios_base::failure` deviates from rules 3+8. Personally, I think it would be reasonable to
add "Exception types should have all their constructors `explicit`," making the latter two of these constructors
conformant but a very small number of multi-argument constructors
(e.g. [`chrono::nonexistent_local_time`](https://eel.is/c++draft/time#zone.exception.nonexist)) deviant.
Notice that most exception types have single-argument constructors which are (by rule 4) `explicit`.

    system_error(error_code);
    explicit failure(const string&, const error_code& = io_errc::stream);
    explicit failure(const char*, const error_code& = io_errc::stream);

The following constructors deviate from rules 3+8. P1163 proposed to fix them (well, all those that
existed when P1163 was written). Notably, the recently added [`basic_syncbuf`](https://eel.is/c++draft/syncstream.syncbuf),
[`basic_osyncstream`](https://eel.is/c++draft/syncstream.osyncstream), etc., do conform to all the rules.

    explicit basic_stringbuf<C,T,A>(const basic_string<C,T,A>&, ios_base::openmode = ios_base::in | ios_base::out);
    explicit basic_stringbuf<C,T,A>(basic_string<C,T,A>&&, ios_base::openmode = ios_base::in | ios_base::out);
    explicit basic_stringbuf<C,T,A>(const basic_string<C,T,S>&, ios_base::openmode = ios_base::in | ios_base::out);
    explicit basic_stringbuf<C,T,A>(const U&, ios_base::openmode = ios_base::in | ios_base::out);
    explicit basic_istringstream<C,T,A>(const basic_string<C,T,A>&, ios_base::openmode = ios_base::in);
    explicit basic_istringstream<C,T,A>(basic_string<C,T,A>&&, ios_base::openmode = ios_base::in);
    explicit basic_istringstream<C,T,A>(const basic_string<C,T,S>&, ios_base::openmode = ios_base::in);
    explicit basic_istringstream<C,T,A>(const U&, ios_base::openmode = ios_base::in);
    explicit basic_ostringstream<C,T,A>(const basic_string<C,T,A>&, ios_base::openmode = ios_base::out);
    explicit basic_ostringstream<C,T,A>(basic_string<C,T,A>&&, ios_base::openmode = ios_base::out);
    explicit basic_ostringstream<C,T,A>(const basic_string<C,T,S>&, ios_base::openmode = ios_base::out);
    explicit basic_ostringstream<C,T,A>(const U&, ios_base::openmode = ios_base::out);
    explicit basic_stringstream<C,T,A>(const basic_string<C,T,A>&, ios_base::openmode = ios_base::out | ios_base::in);
    explicit basic_stringstream<C,T,A>(basic_string<C,T,A>&&, ios_base::openmode = ios_base::out | ios_base::in);
    explicit basic_stringstream<C,T,A>(const basic_string<C,T,S>&, ios_base::openmode = ios_base::out | ios_base::in);
    explicit basic_stringstream<C,T,A>(const U&, ios_base::openmode = ios_base::out | ios_base::in);
    explicit basic_spanbuf<C,T>(span<C>, ios_base::openmode = ios_base::in | ios_base::out);
    explicit basic_ispanstream<C,T>(span<C>, ios_base::openmode = ios_base::in);
    explicit basic_ospanstream<C,T>(span<C>, ios_base::openmode = ios_base::out);
    explicit basic_spanstream<C,T>(span<C>, ios_base::openmode = ios_base::out | ios_base::in);
    explicit basic_ifstream<C,T>(const char*, ios_base::openmode = ios_base::in);
    explicit basic_ifstream<C,T>(const filesystem::path::value_type*, ios_base::openmode = ios_base::in);
    explicit basic_ifstream<C,T>(const string&, ios_base::openmode = ios_base::in);
    explicit basic_ifstream<C,T>(const U&, ios_base::openmode = ios_base::in);
    explicit basic_ofstream<C,T>(const char*, ios_base::openmode = ios_base::out);
    explicit basic_ofstream<C,T>(const filesystem::path::value_type*, ios_base::openmode = ios_base::out);
    explicit basic_ofstream<C,T>(const string&, ios_base::openmode = ios_base::out);
    explicit basic_ofstream<C,T>(const U&, ios_base::openmode = ios_base::out);
    explicit basic_fstream<C,T>(const char*, ios_base::openmode = ios_base::in | ios_base::out);
    explicit basic_fstream<C,T>(const filesystem::path::value_type*, ios_base::openmode = ios_base::in | ios_base::out);
    explicit basic_fstream<C,T>(const string&, ios_base::openmode = ios_base::in | ios_base::out);
    explicit basic_fstream<C,T>(const U&, ios_base::openmode = ios_base::in | ios_base::out);

Many utility types have `explicit` multi-argument constructors taking tag types; these deviate from rules 3+8.
I believe the justification here is "avoids visual ambiguity as to whether we're initializing a `T` or an `AlgebraicOf<T>`."
That is, does `optional<T> t = {in_place, 1, 2};` initialize the contained `T` with `T(1, 2)` or with `T(in_place, 1, 2)`?

    explicit optional(in_place_t, Args&&...);
    explicit optional(in_place_t, initializer_list<U>, Args&&...);
    explicit variant(in_place_type_t<T>, Args&&...);
    explicit variant(in_place_type_t<T>, initializer_list<U>, Args&&...);
    explicit variant(in_place_index_t<I>, Args&&...);
    explicit variant(in_place_index_t<I>, initializer_list<U>, Args&&...);
    explicit unexpected(in_place_t, Args&&...);
    explicit unexpected(in_place_t, initializer_list<U>, Args&&...);
    explicit expected(in_place_t, Args&&...);
    explicit expected(in_place_t, initializer_list<U>, Args&&...);
    explicit expected(unexpect_t, Args&&...);
    explicit expected(unexpect_t, initializer_list<U>, Args&&...);
    explicit single_view<T>(in_place_t, Args&&...);

Some type-erasure types also have `explicit` tagged constructors;
but [`function_ref(nontype_t<f>, U&&)`](https://eel.is/c++draft/func.wrap.ref.class) is conformantly non-`explicit`.
Unlike with the algebraic types, `any t = {in_place_type<T>, 1, 2};` has only one possible meaning,
and so these deviations from rules 3+8 seem unjustified to me.

    explicit any(in_place_type_t<T>, Args&&...);
    explicit any(in_place_type_t<T>, initializer_list<U>, Args&&...);
    explicit move_only_function(in_place_type_t<T>, Args&&...);
    explicit move_only_function(in_place_type_t<T>, initializer_list<U>, Args&&...);
    explicit copyable_function(in_place_type_t<T>, Args&&...);
    explicit copyable_function(in_place_type_t<T>, initializer_list<U>, Args&&...);

Algebraic types justifiably deviate from rules 4, 6, and 8
depending on the `explicit`-ness of the corresponding constructors of their element types:

    explicit(see below) pair<T,U>();
    explicit(see below) pair<T,U>(const T&, const U&);
    explicit(see below) pair<T,U>(A&&, B&&);
    explicit(see below) pair<T,U>(pair<A, B>&);
    explicit(see below) pair<T,U>(const pair<A, B>&);
    explicit(see below) pair<T,U>(pair<A, B>&&);
    explicit(see below) pair<T,U>(const pair<A, B>&&);
    explicit(see below) pair<T,U>(PairLike&&);

    explicit(see below) tuple<Ts...>();
    explicit(see below) tuple<Ts...>(const Ts&...);
    explicit(see below) tuple<Ts...>(Us&&...);
    explicit(see below) tuple<Ts...>(tuple<Us...>&);
    explicit(see below) tuple<Ts...>(const tuple<Us...>&);
    explicit(see below) tuple<Ts...>(tuple<Us...>&&);
    explicit(see below) tuple<Ts...>(const tuple<Us...>&&);
    explicit(see below) tuple<Ts...>(pair<B,C>&);
    explicit(see below) tuple<Ts...>(const pair<B,C>&);
    explicit(see below) tuple<Ts...>(pair<B,C>&&);
    explicit(see below) tuple<Ts...>(const pair<B,C>&&);
    explicit(see below) tuple<Ts...>(TupleLike&&);

On the other hand, the `allocator_arg`-extended constructors of `tuple` seem unjustifiedly complicated.
They might benefit from being made unconditionally `explicit`, with the justification
that they are never meant to be called directly, but only via allocator-aware `push_back`
and the like, which invariably use direct-initialization. Alternatively, they could be
made unconditionally non-`explicit`, thus conforming to rule 8.

    explicit(see below) tuple<Ts...>(allocator_arg_t, const A&);
    explicit(see below) tuple<Ts...>(allocator_arg_t, const A&, const Ts&...);
    explicit(see below) tuple<Ts...>(allocator_arg_t, const A&, Us&&...);
    explicit(see below) tuple<Ts...>(allocator_arg_t, const A&, tuple<Us...>&);
    explicit(see below) tuple<Ts...>(allocator_arg_t, const A&, const tuple<Us...>&);
    explicit(see below) tuple<Ts...>(allocator_arg_t, const A&, tuple<Us...>&&);
    explicit(see below) tuple<Ts...>(allocator_arg_t, const A&, const tuple<Us...>&&);
    explicit(see below) tuple<Ts...>(allocator_arg_t, const A&, pair<B,C>&);
    explicit(see below) tuple<Ts...>(allocator_arg_t, const A&, const pair<B,C>&);
    explicit(see below) tuple<Ts...>(allocator_arg_t, const A&, pair<B,C>&&);
    explicit(see below) tuple<Ts...>(allocator_arg_t, const A&, const pair<B,C>&&);
    explicit(see below) tuple<Ts...>(allocator_arg_t, const A&, TupleLike&&);

The containers have these unjustified deviations from rules 3+8.
Again, notice that these allocator-extended constructors aren't expected to be called explicitly
by the user, so perhaps that could be a justification; but if so, then we should mark many
more of their constructors `explicit` too. P1163 proposed to fix these:

    explicit deque<T,A>(size_type, const A& = A());
    explicit forward_list<T,A>(size_type, const A& = A());
    explicit list<T,A>(size_type, const A& = A());
    explicit vector<T,A>(size_type, const A& = A());
    explicit map<K,T,C,A>(const C&, const A& = A());
    explicit multimap<K,T,C,A>(const C&, const A& = A());
    explicit set<T,C,A>(const C&, const A& = A());
    explicit multiset<T,C,A>(const C&, const A& = A());

Here are other unjustified deviations from rules 3+8. P1163 proposed to fix them:

    explicit unordered_map<K,T,H,E,A>(size_type, const H& = H(), const E& = E(), const A& = A());
    explicit unordered_multimap<K,T,H,E,A>(size_type, const H& = H(), const E& = E(), const A& = A());
    explicit unordered_set<T,H,E,A>(size_type, const H& = H(), const E& = E(), const A& = A());
    explicit unordered_multiset<T,H,E,A>(size_type, const H& = H(), const E& = E(), const A& = A());
    explicit basic_istream<C,T>::sentry(basic_istream&, bool = false);
    explicit basic_ostream<C,T>::sentry(basic_ostream&, bool = false);
    explicit filesystem::file_status(file_type, perms = perms::unknown);
    explicit basic_regex<C,T>(const C*, flag_type = regex_constants::ECMAScript);
    explicit basic_regex(const basic_string<C,U,S>&, flag_type = regex_constants::ECMAScript);
    explicit bitset(const basic_string<C,T,A>&, size_type = 0, size_type = npos, C = '0', C = '1');
    explicit bitset(const basic_string_view<C,T>&, size_type = 0, size_type = npos, C = '0', C = '1');
    explicit bitset(const C*, size_type = 0, size_type = npos, C = '0', C = '1');

My own [P2767](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2767r2.html#explicit-oops)
proposes to fix two recent unjustified deviations from rules 3+8:

    explicit flat_set<T,C,TC>(const TC&, const C& = C());
    explicit flat_multiset<T,C,TC>(const TC&, const C& = C());

`locale::facet` and its children deviate from rules 3+6 and/or 3+8. P1163 chose not to propose
fixing these classes because none of them have a public destructor.

    explicit facet(size_t = 0);
    explicit ctype<C>(size_t = 0);
    explicit ctype_byname<C>(const char*, size_t = 0);
    explicit ctype_byname<C>(const string&, size_t = 0);
    explicit ctype<char>(const mask* = nullptr, bool = false, size_t = 0);
    explicit codecvt<I,E,S>(size_t = 0);
    explicit num_get<C,I>(size_t = 0);
    explicit num_put<C,O>(size_t = 0);
    explicit numpunct<C>(size_t = 0);
    explicit collate<C>(size_t = 0);
    explicit collate_byname<C>(const char*, size_t = 0);
    explicit collate_byname<C>(const string&, size_t = 0);
    explicit time_get<C,I>(size_t = 0);
    explicit time_get_byname<C,I>(const char*, size_t = 0);
    explicit time_get_byname<C,I>(const string&, size_t = 0);
    explicit time_put<C,O>(size_t = 0);
    explicit time_put_byname<C,O>(const char*, size_t = 0);
    explicit time_put_byname<C,O>(const string&, size_t = 0);
    explicit money_get<C,I>(size_t = 0);
    explicit moneypunct<C,I>(size_t = 0);
    explicit messages<C>(size_t = 0);

`string` deviates from rule 8. This being a very promiscuous template,
the deviation might be justified as "avoiding [visual] ambiguity," I'm not sure.

    explicit basic_string<C,T,A>(const StringViewLike&, const A& = A());

`span` justifiably deviates from rule 8, again to prevent `{a, a+3}` from implicitly converting
to `span<const int, 100>`. Here, personally, I'd prefer even more deviation from rule 8 — I think
these constructors should be unconditionally `explicit` — but they're analogous to e.g. `vector`'s
iterator-pair constructor.

    explicit(see below) span<T,N>(It, size_type);
    explicit(see below) span<T,N>(It, Sentinel);

The `mdspan` library deviates a lot from these guidelines.
`extents` is kind of like a tag type, but also kind of like `optional` or `unique_ptr` in
how it implements converting constructors from one kind of `extents` to another.
The converting constructors (justifiably?) deviate from rule 4:

    explicit(see below) extents<I,Es...>(const extents<J,Gs...>&);
    explicit(see below) extents<I,Es...>(span<J,N>);
    explicit(see below) extents<I,Es...>(const array<J,N>&);
    mapping<E>(const E&);
    explicit(see below) mapping<E>(const layout_left::mapping<G>&);
    explicit(see below) mapping<E>(const layout_right::mapping<G>&);
    explicit(see below) mapping<E>(const layout_stride::mapping<G>&);
    explicit(see below) mapping<E>(const LLPM&);
      // likewise for layout_right::mapping, layout_stride::mapping,
      // layout_left_padded::mapping, layout_right_padded::mapping
    explicit(see below) scaled_accessor<F,A>(const scaled_accessor<F,B>&);
    explicit(see below) conjugated_accessor<A>(const conjugated_accessor<B>&);
    explicit(see below) mdspan<T,E,L,A>(const mdspan<U,F,M,B>&);

Then there's a variadic constructor deviating from rules 3+8; but I suspect
that if it _were_ non-`explicit`, it might be ambiguous with the converting constructors
from `span` and/or `array` above:

    explicit extents<I,Es...>(Gs...);

Then there is one constructor template (justifiably?) deviating from rules 3+8:

    explicit mdspan<T,E,L,A>(data_handle_type, Os...);

And two (justifiably?) deviating from rule 8:

    explicit(see below) mdspan<T,E,L,A>(data_handle_type, span<O,N>);
    explicit(see below) mdspan<T,E,L,A>(data_handle_type, const array<O,N>&);

`out_ptr_t` deviates from rule 8 with the justification (AFAIK) that you should never construct an
`out_ptr_t` directly, but only via the factory function `std::out_ptr(p)`. (This is similar to the
above-mentioned possible justification for allocator-extended constructors.)

    explicit out_ptr_t<S,P,Args...>(S&, Args...);
    explicit inout_ptr_t<S,P,Args...>(S&, Args...);

Some types in [thread] deviate from rule 6 and/or rule 8 in ways that seem unjustified,
but also harmless:

    explicit thread(F&&, Args&&...);
    explicit jthread(F&&, Args&&...);
    explicit scoped_lock<Ms...>(Ms&...);
    explicit scoped_lock<Ms...>(adopt_lock_t, Ms&...);
    explicit barrier<F>(ptrdiff_t, F = F());
    explicit stop_callback<C>(const stop_token&, D&&);
    explicit stop_callback<C>(stop_token&&, D&&);

Some distributions in [rand] unjustifiedly deviate from rule 8.
It's not 100% consistent; [`discrete_distribution`](https://eel.is/c++draft/rand.dist.samp.discrete),
`piecewise_constant_distribution`, and `piecewise_linear_distribution` do not deviate.
P1163 proposed to fix all of these:

    explicit uniform_int_distribution<I>(I, I = numeric_limits<I>::max());
    explicit uniform_real_distribution<R>(R, R = 1.0);
    explicit binomial_distribution<I>(I, double = 0.5);
    explicit gamma_distribution<R>(R, R = 1.0);
    explicit weibull_distribution<R>(R, R = 1.0);
    explicit extreme_value_distribution<R>(R, R = 1.0);
    explicit normal_distribution<R>(R, R = 1.0);
    explicit lognormal_distribution<R>(R, R = 1.0);
    explicit cauchy_distribution<R>(R, R = 1.0);
    explicit fisher_f_distribution<R>(R, R = 1.0);

All Ranges view types deviate from rule 8, justified by
[P2711](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2711r1.html).
(But none of them deviate from rule 6.)

    explicit filter_view<V,F>(V, F);
    explicit transform_view<V,F>(V, F);
    explicit take_view<V>(V, range_difference_t<V>);
    explicit take_while_view<V,F>(V, F);
    explicit drop_view<V>(V, range_difference_t<V>);
    explicit drop_while_view<V,F>(V, F);
    explicit join_with_view<V,P>(V, P);
    explicit join_with_view<V,P>(R&&, range_value_t<I>);
    explicit lazy_split_view<V,P>(V, P);
    explicit lazy_split_view<V,P>(R&&, range_value_t<R>);
    explicit split_view<V,P>(V, P);
    explicit split_view<V,P>(R&&, range_value_t<R>);
    explicit concat_view<Views...>(Views...);
    explicit zip_view<Views...>(Views...);
    explicit zip_transform_view<F, Views...>(F, Views...);
    explicit adjacent_transform_view<V,F>(V, F);
    explicit chunk_view<V>(V, range_difference_t<V>);
    explicit slide_view<V>(V, range_difference_t<V>);
    explicit chunk_by_view<V,F>(V, F);
    explicit stride_view<V>(V, range_difference_t<V>);
    explicit cartesian_product_view<V1,Vs...>(V1, Vs...);

The "tag types" in the Standard (which correctly abandon rule 6 for rule 7, and
thus do not deviate from the rules) are:

    explicit sorted_unique_t();
    explicit sorted_equivalent_t();
    explicit full_extent_t();
    explicit allocator_arg_t();
    explicit linalg::column_major_t();
    explicit linalg::row_major_t();
    explicit linalg::upper_triangle_t();
    explicit linalg::lower_triangle_t();
    explicit linalg::implicit_unit_diagonal_t();
    explicit linalg::explicit_diagonal_t();
    explicit from_range_t();
    explicit destroying_delete_t();
    explicit nothrow_t();
    explicit nostopstate_t();
    explicit defer_lock_t();
    explicit try_to_lock_t();
    explicit adopt_lock_t();
    explicit piecewise_construct_t();
    explicit in_place_t();
    explicit in_place_type_t<T>();
    explicit in_place_index_t<N>();
    explicit nontype_t<F>();
    explicit unexpect_t();
    explicit chrono::last_spec();
