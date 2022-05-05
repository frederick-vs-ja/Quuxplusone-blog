---
layout: post
title: 'Two more musings on Clang diagnostics'
date: 2022-05-05 00:01:00 +0000
tags:
  argument-dependent-lookup
  compiler-diagnostics
  hidden-friend-idiom
  llvm
  templates
---

As of this writing, [Chris Di Bella is running a survey on /r/cpp](https://old.reddit.com/r/cpp/comments/uhpa7g/got_a_moment_to_to_share_your_thoughts_on_clangs/)
asking what are the biggest pain points with Clang's diagnostics. In the free-form text entry field, I made
two observations, which I'm now expanding into a full-fledged blog post. (Don't worry — I didn't
type _all_ this into that Google Forms survey!)

First: Fixits for compiler warnings sometimes fail to distinguish between two very different situations:

* Clang self-deprecatingly suggests a way to overrule and silence its warning diagnostic.

      warning: converting the result of '<<' to a boolean; did you
      mean '(x << y) != 0'? [-Wint-in-bool-context]
          return x << y && y < z;
                   ^

* Clang confidently presents a way to fix the actual bug it thinks you've got.

      warning: bitwise negation of a boolean expression always
      evaluates to 'true'; did you mean logical negation? [-Wbool-operation]
          return ~v.empty();
                 ^~~~~~~~~~
                 !

I previously elaborated on this distinction in
["Two musings on the design of compiler warnings"](/blog/2020/09/02/wparentheses/) (2020-09-02).

The good news is that Clang seems to be settling into a convention for at least half of this problem:
a lot of its diagnostics these days end with the set phrase "_...to silence this warning._" When you
see a note like this paired with a "_did you mean..._" warning, it's pretty clear from context that
the "did you mean" part must be presenting a fix for what Clang thinks is a bug. For example:

    warning: result of '10^6' is 12; did you mean '1e6'? [-Wxor-used-as-pow]
        return 10^6;
               ~~^~
               1e6
    note: replace expression with '0xA ^ 6' or use 'xor' instead
    of '^' to silence this warning

However, many Clang warnings still present only one or the other — and worse, they use the
ambiguous "did you mean..." phrasing. It would be interesting to design a consistent way to
present both options to the user in each case, yet somehow without spamming.

Maybe Clang could implement two separate (optional) modes, controlled by command-line options
such as `-Wonly-suppressions` and `-Wonly-fixes`. Then the user could use whichever one they were
interested in at the time.

    $ clang++ test.cpp -Wall -Wonly-fixes
    warning: result of '10^6' is 12; did you mean '1e6'? [-Wxor-used-as-pow]
        return 10^6;
               ~~^~
               1e6

    $ clang++ test.cpp -Wall -Wonly-suppressions
    warning: result of '10^6' is 12; use '0xA^6' or 'xor' to
    silence this warning [-Wxor-used-as-pow]
        return 10^6;
               ~~^~
               0xA^6

I can imagine this idea wouldn't play well with build systems, though. Build systems
(as used for almost all industry code) really want the user to pick a set of flags and
stick to it. In industry, compilation is time-consuming and thus non-interactive:
the compiler gets just one shot at the codebase, and so it needs to present the user
with all possible information up front. Clang's current model is probably the best
we can do for real industry codebases.

----

Second: In my experience, Clang's biggest spammers are template instantiation stacks
and massive operator overload sets. Here's what I mean by a template instantiation stack:

    In file included from /Users/aodwyer/SG14/test/hive_test.cpp:6:
    /Users/aodwyer/SG14/include/sg14/plf_hive.h:703:22: error: invalid operands to binary expression ('plf::hive<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>, std::pmr::polymorphic_allocator<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>>>::hive_iterator<false>' and 'const plf::hive<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>, std::pmr::polymorphic_allocator<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>>>::hive_iterator<false>')
                if (last < *this) {
                    ~~~~ ^ ~~~~~
    /Users/aodwyer/SG14/test/hive_test.cpp:76:5: note: in instantiation of member function 'plf::hive<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>, std::pmr::polymorphic_allocator<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>>>::hive_iterator<false>::distance' requested here
        EXPECT_INVARIANTS(h);
        ^
    /Users/aodwyer/SG14/test/hive_test.cpp:66:25: note: expanded from macro 'EXPECT_INVARIANTS'
        EXPECT_EQ(h.begin().distance(h.end()), h.size()); \
                            ^
    /usr/local/include/gtest/internal/gtest-internal.h:472:44: note: in instantiation of member function 'hivet_BasicInsertClear_Test<plf::hive<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>, std::pmr::polymorphic_allocator<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>>>>::TestBody' requested here
      Test* CreateTest() override { return new TestClass; }
                                               ^
    /usr/local/include/gtest/internal/gtest-internal.h:740:13: note: in instantiation of member function 'testing::internal::TestFactoryImpl<hivet_BasicInsertClear_Test<plf::hive<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>, std::pmr::polymorphic_allocator<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>>>>>::CreateTest' requested here
            new TestFactoryImpl<TestClass>);
                ^
    /usr/local/include/gtest/internal/gtest-internal.h:744:57: note: in instantiation of member function 'testing::internal::TypeParameterizedTest<hivet, testing::internal::TemplateSel<hivet_BasicInsertClear_Test>, testing::internal::Types<plf::hive<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>, std::pmr::polymorphic_allocator<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>>>>>::Register' requested here
                                     typename Types::Tail>::Register(prefix,
                                                            ^
    /usr/local/include/gtest/internal/gtest-internal.h:744:57: note: in instantiation of member function 'testing::internal::TypeParameterizedTest<hivet, testing::internal::TemplateSel<hivet_BasicInsertClear_Test>, testing::internal::Types<plf::hive<int, std::pmr::polymorphic_allocator<int>>, plf::hive<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>, std::pmr::polymorphic_allocator<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>>>>>::Register' requested here
    /usr/local/include/gtest/internal/gtest-internal.h:744:57: note: in instantiation of member function 'testing::internal::TypeParameterizedTest<hivet, testing::internal::TemplateSel<hivet_BasicInsertClear_Test>, testing::internal::Types<plf::hive<std::string>, plf::hive<int, std::pmr::polymorphic_allocator<int>>, plf::hive<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>, std::pmr::polymorphic_allocator<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>>>>>::Register' requested here
    /usr/local/include/gtest/internal/gtest-internal.h:744:57: note: in instantiation of member function 'testing::internal::TypeParameterizedTest<hivet, testing::internal::TemplateSel<hivet_BasicInsertClear_Test>, testing::internal::Types<plf::hive<int, std::allocator<int>, plf::hive_priority::memory_use>, plf::hive<std::string>, plf::hive<int, std::pmr::polymorphic_allocator<int>>, plf::hive<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>, std::pmr::polymorphic_allocator<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>>>>>::Register' requested here
    /usr/local/include/gtest/internal/gtest-internal.h:744:57: note: in instantiation of member function 'testing::internal::TypeParameterizedTest<hivet, testing::internal::TemplateSel<hivet_BasicInsertClear_Test>, testing::internal::Types<plf::hive<int>, plf::hive<int, std::allocator<int>, plf::hive_priority::memory_use>, plf::hive<std::string>, plf::hive<int, std::pmr::polymorphic_allocator<int>>, plf::hive<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>, std::pmr::polymorphic_allocator<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>>>>>::Register' requested here
    /Users/aodwyer/SG14/test/hive_test.cpp:70:1: note: in instantiation of member function 'testing::internal::TypeParameterizedTest<hivet, testing::internal::TemplateSel<hivet_BasicInsertClear_Test>, testing::internal::Types<plf::hive<unsigned char>, plf::hive<int>, plf::hive<int, std::allocator<int>, plf::hive_priority::memory_use>, plf::hive<std::string>, plf::hive<int, std::pmr::polymorphic_allocator<int>>, plf::hive<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>, std::pmr::polymorphic_allocator<std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>>>>>::Register' requested here
    TYPED_TEST(hivet, BasicInsertClear)
    ^
    /usr/local/include/gtest/gtest-typed-test.h:212:27: note: expanded from macro 'TYPED_TEST'
                  CaseName)>::Register("",                                        \
                              ^

The maximum length of this stack of notes is controlled by Clang's command-line option
<span style="white-space: nowrap;">`-ftemplate-backtrace-limit=`,</span> which defaults to `10`. Maybe I should start setting
<span style="white-space: nowrap;">`-ftemplate-backtrace-limit=1`</span> in all my builds, and see if I like that experience better.
It feels like usually all of the notes beginning "_in instantiation of..._" are completely useless to me.
The only important error is right at the top, where it tells me that `last < *this` uses an
operator that doesn't exist, and it tells me the operand types. Everything below that point
is basically explaining "how we got here," which usually is unnecessary, because my bug is
rarely in "how I got here" — it's almost always something to do with the offending line itself.

And in case you think C++20 Concepts makes the situation better — no, it doesn't. At best,
constrained templates merely replace the stack of "_in instantiation of..._" notes with
a new stack of "_while substituting template arguments into constraint expression..._" notes.
See [P2538 "ADL-proof `std::projected`," Appendix A](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2538r1.html#appendix-a),
for a real-world example involving all three major compilers. <span style="white-space: nowrap;">(`-ftemplate-backtrace-limit=`</span>
controls the length of this stack as well.)

I must admit that sometimes — rarely, but sometimes! — my bug really is in "how I got here":
there'll be nothing wrong with the offending line itself, but rather somewhere up the call stack
my metaprogramming took the wrong branch. Like, maybe I wrote `if constexpr (std::forward_iterator<It>)`
when I actually meant `if constexpr (!std::forward_iterator<It>)`. In that case, having the
instantiation stack might be useful. So again, I'm not sure that the right answer here is
any different from what we have today, except perhaps to reduce <span style="white-space: nowrap;">`-ftemplate-backtrace-limit=`</span>
from 10 down to just 1 or 2.

Here's what I mean by a massive operator overload set:

    In file included from /Users/aodwyer/SG14/test/hive_test.cpp:8:
    /usr/local/include/gtest/gtest.h:1545:11: error: invalid operands to binary expression ('const plf::hive<S>' and 'const plf::hive<S>')
      if (lhs == rhs) {
          ~~~ ^  ~~~
    /usr/local/include/gtest/gtest.h:1564:12: note: in instantiation of function template specialization 'testing::internal::CmpHelperEQ<plf::hive<S>, plf::hive<S>>' requested here
        return CmpHelperEQ(lhs_expression, rhs_expression, lhs, rhs);
               ^
    /Users/aodwyer/SG14/test/hive_test.cpp:107:5: note: in instantiation of function template specialization 'testing::internal::EqHelper::Compare<plf::hive<S>, plf::hive<S>, nullptr>' requested here
        EXPECT_EQ(h, h);
        ^
    /usr/local/include/gtest/gtest.h:2044:54: note: expanded from macro 'EXPECT_EQ'
      EXPECT_PRED_FORMAT2(::testing::internal::EqHelper::Compare, val1, val2)
                                                         ^
    /usr/include/c++/v1/__variant/monostate.h:40:16: note: candidate function not viable: no known conversion from 'const plf::hive<S>' to 'std::monostate' for 1st argument
    constexpr bool operator==(monostate, monostate) noexcept { return true; }
                   ^
    /usr/include/c++/v1/system_error:389:1: note: candidate function not viable: no known conversion from 'const plf::hive<S>' to 'const std::error_code' for 1st argument
    operator==(const error_code& __x, const error_code& __y) _NOEXCEPT
    ^
    /usr/include/c++/v1/system_error:396:1: note: candidate function not viable: no known conversion from 'const plf::hive<S>' to 'const std::error_code' for 1st argument
    operator==(const error_code& __x, const error_condition& __y) _NOEXCEPT
    ^
    /usr/include/c++/v1/system_error:396:1: note: candidate function (with reversed parameter order) not viable: no known conversion from 'const plf::hive<S>' to 'const std::error_condition' for 1st argument
    /usr/include/c++/v1/system_error:404:1: note: candidate function not viable: no known conversion from 'const plf::hive<S>' to 'const std::error_condition' for 1st argument

    operator==(const error_condition& __x, const error_code& __y) _NOEXCEPT
    ^
    /usr/include/c++/v1/system_error:404:1: note: candidate function (with reversed parameter order) not viable: no known conversion from 'const plf::hive<S>' to 'const std::error_code' for 1st argument
    /usr/include/c++/v1/system_error:411:1: note: candidate function not viable: no known conversion from 'const plf::hive<S>' to 'const std::error_condition' for 1st argument
    operator==(const error_condition& __x, const error_condition& __y) _NOEXCEPT
    ^
    /usr/local/include/gtest/gtest.h:1536:13: note: candidate function not viable: no known conversion from 'const plf::hive<S>' to 'testing::internal::faketype' for 1st argument
    inline bool operator==(faketype, faketype) { return true; }
                ^
    /usr/include/c++/v1/__utility/pair.h:328:1: note: candidate template ignored: could not match 'pair' against 'hive'
    operator==(const pair<_T1,_T2>& __x, const pair<_T1,_T2>& __y)
    ^
    /usr/include/c++/v1/tuple:1343:1: note: candidate template ignored: could not match 'tuple' against 'hive'
    operator==(const tuple<_Tp...>& __x, const tuple<_Up...>& __y)
    ^
    /usr/include/c++/v1/tuple:1343:1: note: candidate template ignored: could not match 'tuple' against 'hive'
    /usr/include/c++/v1/variant:1627:16: note: candidate template ignored: could not match 'variant' against 'hive'
    constexpr bool operator==(const variant<_Types...>& __lhs,
                   ^
    /usr/include/c++/v1/__iterator/istream_iterator.h:85:1: note: candidate template ignored: could not match 'istream_iterator' against 'hive'
    operator==(const istream_iterator<_Tp, _CharT, _Traits, _Distance>& __x,
    ^
    /usr/include/c++/v1/__iterator/istreambuf_iterator.h:105:6: note: candidate template ignored: could not match 'istreambuf_iterator' against 'hive'
    bool operator==(const istreambuf_iterator<_CharT,_Traits>& __a,
         ^
    /usr/include/c++/v1/__iterator/move_iterator.h:237:6: note: candidate template ignored: could not match 'move_iterator' against 'hive'
    bool operator==(const move_iterator<_Iter1>& __x, const move_iterator<_Iter2>& __y)
         ^
    /usr/include/c++/v1/__iterator/move_iterator.h:237:6: note: candidate template ignored: could not match 'move_iterator' against 'hive'
    /usr/include/c++/v1/__iterator/reverse_iterator.h:204:1: note: candidate template ignored: could not match 'reverse_iterator' against 'hive'
    operator==(const reverse_iterator<_Iter1>& __x, const reverse_iterator<_Iter2>& __y)
    ^
    /usr/include/c++/v1/__iterator/reverse_iterator.h:204:1: note: candidate template ignored: could not match 'reverse_iterator' against 'hive'
    /usr/include/c++/v1/__iterator/wrap_iter.h:158:6: note: candidate template ignored: could not match '__wrap_iter' against 'hive'
    bool operator==(const __wrap_iter<_Iter1>& __x, const __wrap_iter<_Iter1>& __y) _NOEXCEPT
         ^
    /usr/include/c++/v1/__iterator/wrap_iter.h:165:6: note: candidate template ignored: could not match '__wrap_iter' against 'hive'
    bool operator==(const __wrap_iter<_Iter1>& __x, const __wrap_iter<_Iter2>& __y) _NOEXCEPT
         ^
    /usr/include/c++/v1/__iterator/wrap_iter.h:165:6: note: candidate template ignored: could not match '__wrap_iter' against 'hive'
    /usr/include/c++/v1/__memory/allocator.h:256:6: note: candidate template ignored: could not match 'allocator' against 'hive'
    bool operator==(const allocator<_Tp>&, const allocator<_Up>&) _NOEXCEPT {return true;}
         ^
    /usr/include/c++/v1/__memory/allocator.h:256:6: note: candidate template ignored: could not match 'allocator' against 'hive'
    /usr/include/c++/v1/__memory/unique_ptr.h:577:1: note: candidate template ignored: could not match 'unique_ptr' against 'hive'
    operator==(const unique_ptr<_T1, _D1>& __x, const unique_ptr<_T2, _D2>& __y) {return __x.get() == __y.get();}
    ^
    /usr/include/c++/v1/__memory/unique_ptr.h:577:1: note: candidate template ignored: could not match 'unique_ptr' against 'hive'
    /usr/include/c++/v1/__memory/unique_ptr.h:613:1: note: candidate template ignored: could not match 'unique_ptr' against 'hive'
    operator==(const unique_ptr<_T1, _D1>& __x, nullptr_t) _NOEXCEPT
    ^
    /usr/include/c++/v1/__memory/unique_ptr.h:613:1: note: candidate template ignored: could not match 'unique_ptr' against 'hive'
    /usr/include/c++/v1/__memory/unique_ptr.h:621:1: note: candidate template ignored: could not match 'unique_ptr' against 'hive'
    operator==(nullptr_t, const unique_ptr<_T1, _D1>& __x) _NOEXCEPT
    ^
    /usr/include/c++/v1/__memory/unique_ptr.h:621:1: note: candidate template ignored: could not match 'unique_ptr' against 'hive'
    /usr/include/c++/v1/__memory/shared_ptr.h:1180:1: note: candidate template ignored: could not match 'shared_ptr' against 'hive'
    operator==(const shared_ptr<_Tp>& __x, const shared_ptr<_Up>& __y) _NOEXCEPT
    ^
    /usr/include/c++/v1/__memory/shared_ptr.h:1180:1: note: candidate template ignored: could not match 'shared_ptr' against 'hive'
    /usr/include/c++/v1/__memory/shared_ptr.h:1234:1: note: candidate template ignored: could not match 'shared_ptr' against 'hive'
    operator==(const shared_ptr<_Tp>& __x, nullptr_t) _NOEXCEPT
    ^
    /usr/include/c++/v1/__memory/shared_ptr.h:1234:1: note: candidate template ignored: could not match 'shared_ptr' against 'hive'
    /usr/include/c++/v1/__memory/shared_ptr.h:1242:1: note: candidate template ignored: could not match 'shared_ptr' against 'hive'
    operator==(nullptr_t, const shared_ptr<_Tp>& __x) _NOEXCEPT
    ^
    /usr/include/c++/v1/__memory/shared_ptr.h:1242:1: note: candidate template ignored: could not match 'shared_ptr' against 'hive'
    /usr/include/c++/v1/__functional/function.h:1219:1: note: candidate template ignored: could not match 'function' against 'hive'
    operator==(const function<_Rp(_ArgTypes...)>& __f, nullptr_t) _NOEXCEPT {return !__f;}
    ^
    /usr/include/c++/v1/__functional/function.h:1219:1: note: candidate template ignored: could not match 'function' against 'hive'
    /usr/include/c++/v1/__functional/function.h:1224:1: note: candidate template ignored: could not match 'function' against 'hive'
    operator==(nullptr_t, const function<_Rp(_ArgTypes...)>& __f) _NOEXCEPT {return !__f;}
    ^
    /usr/include/c++/v1/__functional/function.h:1224:1: note: candidate template ignored: could not match 'function' against 'hive'
    /usr/include/c++/v1/array:374:1: note: candidate template ignored: could not match 'array' against 'hive'
    operator==(const array<_Tp, _Size>& __x, const array<_Tp, _Size>& __y)
    ^
    /usr/include/c++/v1/optional:1193:1: note: candidate template ignored: could not match 'optional' against 'hive'
    operator==(const optional<_Tp>& __x, const optional<_Up>& __y)
    ^
    /usr/include/c++/v1/optional:1193:1: note: candidate template ignored: could not match 'optional' against 'hive'
    /usr/include/c++/v1/optional:1286:1: note: candidate template ignored: could not match 'optional' against 'hive'
    operator==(const optional<_Tp>& __x, nullopt_t) noexcept
    ^
    /usr/include/c++/v1/optional:1286:1: note: candidate template ignored: could not match 'optional' against 'hive'
    /usr/include/c++/v1/optional:1294:1: note: candidate template ignored: could not match 'optional' against 'hive'
    operator==(nullopt_t, const optional<_Tp>& __x) noexcept
    ^
    /usr/include/c++/v1/optional:1294:1: note: candidate template ignored: could not match 'optional' against 'hive'
    /usr/include/c++/v1/optional:1387:1: note: candidate template ignored: could not match 'optional' against 'hive'
    operator==(const optional<_Tp>& __x, const _Up& __v)
    ^
    /usr/include/c++/v1/optional:1387:1: note: candidate template ignored: could not match 'optional' against 'hive'
    /usr/include/c++/v1/optional:1399:1: note: candidate template ignored: could not match 'optional' against 'hive'
    operator==(const _Tp& __v, const optional<_Up>& __x)
    ^
    /usr/include/c++/v1/optional:1399:1: note: candidate template ignored: could not match 'optional' against 'hive'
    /usr/include/c++/v1/__ios/fpos.h:67:6: note: candidate template ignored: could not match 'fpos' against 'hive'
    bool operator==(const fpos<_StateT>& __x, const fpos<_StateT>& __y) {
         ^
    /usr/include/c++/v1/string_view:745:6: note: candidate template ignored: could not match 'basic_string_view' against 'hive'
    bool operator==(basic_string_view<_CharT, _Traits> __lhs,
         ^
    /usr/include/c++/v1/string_view:756:6: note: candidate template ignored: could not match 'basic_string_view' against 'hive'
    bool operator==(basic_string_view<_CharT, _Traits> __lhs,
         ^
    /usr/include/c++/v1/string_view:756:6: note: candidate template ignored: could not match 'basic_string_view' against 'hive'
    /usr/include/c++/v1/string_view:765:6: note: candidate template ignored: could not match 'basic_string_view' against 'hive'
    bool operator==(typename common_type<basic_string_view<_CharT, _Traits> >::type __lhs,
         ^
    /usr/include/c++/v1/string_view:765:6: note: candidate template ignored: could not match 'basic_string_view' against 'hive'
    /usr/include/c++/v1/string:3991:1: note: candidate template ignored: could not match 'basic_string' against 'hive'
    operator==(const basic_string<_CharT, _Traits, _Allocator>& __lhs,
    ^
    /usr/include/c++/v1/string:4003:1: note: candidate template ignored: could not match 'basic_string' against 'hive'
    operator==(const basic_string<char, char_traits<char>, _Allocator>& __lhs,
    ^
    /usr/include/c++/v1/string:4022:1: note: candidate template ignored: could not match 'const _CharT *' against 'plf::hive<S>'
    operator==(const _CharT* __lhs,
    ^
    /usr/include/c++/v1/string:4022:1: note: candidate template ignored: could not match 'const _CharT *' against 'plf::hive<S>'
    /usr/include/c++/v1/string:4035:1: note: candidate template ignored: could not match 'basic_string' against 'hive'
    operator==(const basic_string<_CharT,_Traits,_Allocator>& __lhs,
    ^
    /usr/include/c++/v1/string:4035:1: note: candidate template ignored: could not match 'basic_string' against 'hive'
    /usr/include/c++/v1/vector:3342:1: note: candidate template ignored: could not match 'vector' against 'hive'
    operator==(const vector<_Tp, _Allocator>& __x, const vector<_Tp, _Allocator>& __y)
    ^
    /usr/include/c++/v1/map:1665:1: note: candidate template ignored: could not match 'map' against 'hive'
    operator==(const map<_Key, _Tp, _Compare, _Allocator>& __x,
    ^
    /usr/include/c++/v1/map:2265:1: note: candidate template ignored: could not match 'multimap' against 'hive'
    operator==(const multimap<_Key, _Tp, _Compare, _Allocator>& __x,
    ^
    /usr/include/c++/v1/set:960:1: note: candidate template ignored: could not match 'set' against 'hive'
    operator==(const set<_Key, _Compare, _Allocator>& __x,
    ^
    /usr/include/c++/v1/set:1492:1: note: candidate template ignored: could not match 'multiset' against 'hive'
    operator==(const multiset<_Key, _Compare, _Allocator>& __x,
    ^
    /usr/include/c++/v1/__random/discard_block_engine.h:150:1: note: candidate template ignored: could not match 'discard_block_engine' against 'hive'
    operator==(const discard_block_engine<_Eng, _Pp, _Rp>& __x,
    ^
    /usr/include/c++/v1/__random/independent_bits_engine.h:228:1: note: candidate template ignored: could not match 'independent_bits_engine' against 'hive'
    operator==(
    ^
    /usr/include/c++/v1/__random/shuffle_order_engine.h:219:1: note: candidate template ignored: could not match 'shuffle_order_engine' against 'hive'
    operator==(
    ^
    /usr/include/c++/v1/__random/mersenne_twister_engine.h:420:1: note: candidate template ignored: could not match 'mersenne_twister_engine' against 'hive'
    operator==(const mersenne_twister_engine<_UInt, _Wp, _Np, _Mp, _Rp, _Ap, _Up, _Dp, _Sp,
    ^
    /usr/include/c++/v1/__random/subtract_with_carry_engine.h:255:1: note: candidate template ignored: could not match 'subtract_with_carry_engine' against 'hive'
    operator==(
    ^
    1 error generated.

That's right — Clang calls this "_one_ error generated"!
The spew of notes identifies 48 different `operator==` candidates.
To mitigate the spam, one simple idea would be to implement a command-line option <span style="white-space: nowrap;">`-fcandidate-set-limit=`</span>
with a default of, let's say, `10`. That would cut the number of lines of spam above by more than 75%.
But, as we just saw with <span style="white-space: nowrap;">`-ftemplate-backtrace-limit=`,</span> merely adding a command-line option to limit
the compiler's unhelpful output isn't enough: we ought to try to make its output more helpful to begin with.

To improve the user's experience here, the library vendor could turn all of these `operator==` overloads
into hidden friends. In many cases that's permitted by the Standard these days; in other cases a vendor's
non-conformance might be forgiven as a clear improvement over the status quo.
See ["An example of the Barton–Nackman trick"](/blog/2020/12/09/barton-nackman-in-practice/) (2020-12-09).

However, even if the library vendor doesn't play along, Clang itself could emit diagnostics _as if_
all these operators were hidden friends! That is, perhaps Clang could simply not bother to note any
of these candidates that wouldn't have been found if they had been hidden friends. Or at least it
could prioritize candidates involving associated types so that they show up higher in the spew,
before the <span style="white-space: nowrap;">`-fcandidate-set-limit=`</span> kicks in.

> By the way, you might be surprised
> that ADL would find all of these `operator==` overloads in namespace `std`,
> given that the actual arguments we're passing are of type `plf::hive<S>`.
> The trick is that `plf::hive`, as an STL-style container type, takes
> a defaulted template argument: it's really `plf::hive<S, std::allocator<S>>`.
> This causes `namespace std` to become an associated namespace, even though
> we don't really want it to be.
