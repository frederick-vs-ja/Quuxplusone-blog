// Instructions for use:
// clang++ -std=c++14 -O2 -march=native -DNDEBUG \
//     2023-03-05-oeis-a360447-revised.cpp -DMAX=3672819156
// ./a.out
//
// When you see "next update at i=XYZ", that's the number you should plug into -DMAX=
// for the next compilation. Plugging in any smaller -DMAX= will be completely useless.
//
// When MAX exceeds (1<<39) = 562'000'000'000'000, a `static_assert` below will fail, and
// you'll need to add more bits to the `Int` type. Do that by manually editing this
// file: just change
//     using Int = IntN<5>;
// to
//     using Int = IntN<6>;

#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <type_traits>

int elapsed_sec() {
    static auto start = std::chrono::steady_clock::now();
    auto finish = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(finish - start).count();
}

#ifndef MAX
 #error "Set -DMAX=100'000'000 on the command line!"
#endif

template<int Bytes>
struct IntN {
    IntN() = default;
    constexpr IntN(size_t x) {
        for (int i=0; i < Bytes; ++i) {
            data_[i] = x;
            x >>= 8;
        }
    }
    constexpr operator size_t() const {
        size_t result = 0;
        for (int i=0; i < Bytes; ++i) {
            result = (result << 8) | data_[Bytes-i-1];
        }
        return result;
    }
private:
    unsigned char data_[Bytes];
};

using Int = IntN<5>;
static_assert(Int(MAX) + Int(MAX) == size_t(MAX) + size_t(MAX), "Increase the size of Int");

size_t absdiff(size_t a, size_t b) { return (a < b) ? (b - a) : (a - b); }

bool can_find_better_insertion_point(const Int *a, size_t i, size_t sum, size_t diff) {
    for (size_t p = 0; p < i - 1; ++p) {
        size_t q = a[p];
        if (p + q == sum && absdiff(p, q) < diff) {
            return true;
        }
    }
    return false;
}

// We maintain an array of MAX ints.
// For each integer less than the current `i`, a[i] holds the successor of `i` in the list.
// For each integer greater than `i`, a[i] holds the predecessor of `i` if it were to be inserted right now.

size_t print_update(const Int *a, size_t i) {
    printf("--------------------------------\n");
    size_t p = 0;
    while (true) {
        printf("%zu, ", p);
        size_t q = a[p];
        size_t sum = p + q;
        if (sum <= i) {
            // This sum has already been inserted elsewhere; q is stable.
        } else {
            printf("  (i=%zu, next update at i=%zu, elapsed=%ds)\n", i, sum, elapsed_sec());
            return sum;
        }
        p = q;
    }
}

void print_final_update(const Int *a, size_t i) {
    printf("--------------------------------\n");
    size_t p = 0;
    size_t count = 0;
    while (true) {
        count += 1;
        printf("%zu, ", p);
        size_t q = a[p];
        size_t sum = p + q;
        if (sum <= i) {
            // This sum has already been inserted elsewhere; q is stable.
        } else if (can_find_better_insertion_point(a, i, sum, absdiff(p, q))) {
            // This sum will be inserted elsewhere; q is stable.
        } else {
            printf("  (i=%zu, next update at i=%zu, elapsed=%ds)\n", i, sum, elapsed_sec());
            printf("\nStable terms a(0)-a(%zu) appear above.\n", count - 1);
            printf("As of i=%zu, the list continues with these (not-yet-stable) terms:", i);
            for (int j=0; j < 10; ++j) {
                p = a[p];
                printf(" %zu,", p);
            }
            printf("\n\n");
            return;
        }
        p = q;
    }
}

void maybe_map(Int *a, size_t p, size_t q) {
    size_t sum = p + q;
    if (sum >= MAX) return;
    size_t oldp = a[sum];
    if (oldp == 0) {
        a[sum] = p;
    } else {
        // Otherwise we already found the place to insert this sum.
        assert(p > oldp || q > oldp);
        assert(absdiff(p, q) > absdiff(oldp, sum - oldp));
    }
}

int main()
{
    Int *a = (Int *)std::calloc(MAX+1, sizeof(Int));
    a[0] = 1;  // the successor of 0 in {0,1,2} is 1
    a[1] = 2;  // the successor of 1 in {0,1,2} is 2
    a[2] = 0;  // the successor of 2 in {0,1,2} is non-existent
    a[3] = 1;  // insert 3 after 1
    size_t back = 2;  // the last element of {0,1,2} is 2
    size_t nextupdate = 100'000;
    for (size_t i = 3; i <= MAX; ++i) {
        size_t p = a[i];
        if (p == 0) {
            p = back;
            // Insert `i` after `p` in the list.
            assert(a[p] == 0);
            a[p] = i;
            assert(a[i] == 0);
            back = i;
            maybe_map(a, p, i);
        } else {
            Int q = i - p;
            assert(q == a[p]);
            // Now insert `i` between `p` and `q` in the list.
            a[i] = a[p];
            a[p] = i;
            maybe_map(a, p, i);
            maybe_map(a, i, q);
            if (i >= nextupdate) {
                nextupdate = print_update(a, i);
            }
        }
    }
    print_final_update(a, MAX);
}
