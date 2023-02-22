// Your patched library can use this stub code if you like.
// struct TR;
// namespace std {
//   template<class> bool is_trivially_relocatable_v = false;
//   template<> bool is_trivially_relocatable_v<TR> = true;
// }

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <memory>
#include <ranges>
#include <type_traits>

struct TR {
    TR(std::unique_ptr<int> p) : p(std::move(p)) {}
    std::unique_ptr<int> p;
    int i = 0;
};
//static_assert(std::is_trivially_relocatable_v<TR>);

inline bool a_contains(TR *a, std::initializer_list<int> il) {
    return std::equal(a, a+6, il.begin(), il.end(), [](auto& tr, int i) {
        return *tr.p == i;
    });
}

void must_use_memmove_1(TR *first, TR *mid, TR *last) {
    // The codegen for this function mustn't involve any calls to "operator delete", not even in untaken branches.
    std::rotate(first, mid, last);
}

void must_use_memmove_2(TR *first, TR *mid, TR *last) {
    // The codegen for this function mustn't involve any calls to "operator delete", not even in untaken branches.
    std::ranges::rotate(first, mid, last);
}

void must_use_memmove_3(TR *first, TR *last, int k) {
    // The codegen for this function mustn't involve any calls to "operator delete", not even in untaken branches.
    std::partition(first, last, [&](auto& tr) { return *tr.p == k; });
}

void must_use_memmove_4(TR *first, TR *last, int k) {
    // The codegen for this function mustn't involve any calls to "operator delete", not even in untaken branches.
    std::ranges::partition(first, last, [&](auto& tr) { return *tr.p == k; });
}

void must_use_memmove_5(TR *first, TR *last, TR *other) {
    // The codegen for this function mustn't involve any calls to "operator delete", not even in untaken branches.
    std::swap_ranges(first, last, other);
}

void must_not_fail_6() {
    // These assertions must pass, not fail.
    struct D {
        [[no_unique_address]] TR tr;
        int j;
    };
    D a = { std::make_unique<int>(1), 1 };
    D b = { std::make_unique<int>(2), 2 };
    std::swap(a.tr, b.tr);
    assert(*a.tr.p == 2 && *b.tr.p == 1);
    assert(a.tr.i == 0 && b.tr.i == 0);
    assert(a.j == 1 && b.j == 2);
}

void must_not_fail_7() {
    // These assertions must pass, not fail.
    struct D {
        [[no_unique_address]] TR tr;
        int j;
    };
    D a[3] = {
        { std::make_unique<int>(1), 1 },
        { std::make_unique<int>(2), 2 },
        { std::make_unique<int>(3), 3 }
    };
    auto transformed = a | std::views::transform([](D& d) -> TR& { return d.tr; });
    std::rotate(transformed.begin(), transformed.begin() + 1, transformed.end());
    assert(*a[0].tr.p == 2);
    assert(*a[1].tr.p == 3);
    assert(*a[2].tr.p == 1);
    assert(a[0].j == 1);
    assert(a[1].j == 2);
    assert(a[2].j == 3);
}

void should_use_memmove_8(TR *first, TR *mid, TR *last) {
    // BONUS: The codegen for this function shouldn't involve any calls to "operator delete", not even in untaken branches.
    // But this is an extra bonus challenge; you'll win the prize even if you don't achieve this one.
    using R = std::reverse_iterator<TR*>;
    std::rotate(R(last), R(mid), R(first));
}

int main() {
    TR a[] = {
        {std::make_unique<int>(1)},
        {std::make_unique<int>(2)},
        {std::make_unique<int>(3)},
        {std::make_unique<int>(4)},
        {std::make_unique<int>(5)},
        {std::make_unique<int>(6)},
    };
    TR other[6] = {
        {std::make_unique<int>(7)},
        {std::make_unique<int>(8)},
        {std::make_unique<int>(9)},
        {std::make_unique<int>(10)},
        {std::make_unique<int>(11)},
        {std::make_unique<int>(12)},
    };
    must_use_memmove_1(a, a+2, a+6);
    assert(a_contains(a, {3,4,5,6,1,2}));
    must_use_memmove_1(a, a+3, a+6);
    assert(a_contains(a, {6,1,2,3,4,5}));
    must_use_memmove_3(a, a+6, 3);
    assert(a_contains(a, {3,1,2,6,4,5}));
    must_use_memmove_4(a, a+6, 4);
    assert(a_contains(a, {4,1,2,6,3,5}));
    must_use_memmove_5(a, a+6, other);
    assert(a_contains(a, {7,8,9,10,11,12}));
    must_not_fail_6();
    must_not_fail_7();
    should_use_memmove_8(a, a+2, a+6);
    assert(a_contains(a, {9,10,11,12,7,8}));
    puts("All tests passed!");
}
