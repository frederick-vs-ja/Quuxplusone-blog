/*
  Requires the libc++ patch from
    https://github.com/Quuxplusone/llvm-project/commit/unordered-multiset-quick-insert

  You should compile eight different benchmark executables:
    clang++ this.cpp -std=c++17 -O2 `pkg-config --cflags --libs benchmark` \
        -D_LIBCPP_UNORDERED_MULTISET_QUICK_INSERT=0 \
        -DUSE_FLAT_DISTRIBUTION=0 \
        -DTENTHOUSAND=10000 -o old-zipf-small.exe
    clang++ this.cpp -std=c++17 -O2 `pkg-config --cflags --libs benchmark` \
        -D_LIBCPP_UNORDERED_MULTISET_QUICK_INSERT=1 \
        -DUSE_FLAT_DISTRIBUTION=0 \
        -DTENTHOUSAND=10000 -o new-zipf-small.exe
    clang++ this.cpp -std=c++17 -O2 `pkg-config --cflags --libs benchmark` \
        -D_LIBCPP_UNORDERED_MULTISET_QUICK_INSERT=0 \
        -DUSE_FLAT_DISTRIBUTION=1 \
        -DTENTHOUSAND=10000 -o old-flat-small.exe
    clang++ this.cpp -std=c++17 -O2 `pkg-config --cflags --libs benchmark` \
        -D_LIBCPP_UNORDERED_MULTISET_QUICK_INSERT=1 \
        -DUSE_FLAT_DISTRIBUTION=1 \
        -DTENTHOUSAND=10000 -o new-flat-small.exe
    clang++ this.cpp -std=c++17 -O2 `pkg-config --cflags --libs benchmark` \
        -D_LIBCPP_UNORDERED_MULTISET_QUICK_INSERT=0 \
        -DUSE_FLAT_DISTRIBUTION=0 \
        -DTENTHOUSAND=100000 -o old-zipf-big.exe
    clang++ this.cpp -std=c++17 -O2 `pkg-config --cflags --libs benchmark` \
        -D_LIBCPP_UNORDERED_MULTISET_QUICK_INSERT=1 \
        -DUSE_FLAT_DISTRIBUTION=0 \
        -DTENTHOUSAND=100000 -o new-zipf-big.exe
    clang++ this.cpp -std=c++17 -O2 `pkg-config --cflags --libs benchmark` \
        -D_LIBCPP_UNORDERED_MULTISET_QUICK_INSERT=0 \
        -DUSE_FLAT_DISTRIBUTION=1 \
        -DTENTHOUSAND=100000 -o old-flat-big.exe
    clang++ this.cpp -std=c++17 -O2 `pkg-config --cflags --libs benchmark` \
        -D_LIBCPP_UNORDERED_MULTISET_QUICK_INSERT=1 \
        -DUSE_FLAT_DISTRIBUTION=1 \
        -DTENTHOUSAND=100000 -o new-flat-big.exe
*/

#include <algorithm>
#include <cassert>
#include <map>
#include <random>
#include <unordered_set>
#include <vector>

#include <benchmark/benchmark.h>

#ifndef TENTHOUSAND
 #define TENTHOUSAND 10'000
#endif

std::vector<int> g_data = []() {
    std::vector<std::pair<size_t, unsigned>> v;
    std::mt19937 g;
    for (int i = 0; i < TENTHOUSAND; ++i) {
        unsigned value = g();
#if USE_FLAT_DISTRIBUTION
        int ncopies = 37;
#else
        int ncopies = (i < 10) ? TENTHOUSAND/10 : (i < 100) ? TENTHOUSAND/100 : (i < 1000) ? TENTHOUSAND/1000 : TENTHOUSAND/10'000;
#endif
        for (int j = 0; j < ncopies; ++j) {
            v.emplace_back(g(), value);
        }
    }
    std::stable_sort(v.begin(), v.end());
    std::vector<int> result;
    for (const auto& p : v) {
        result.push_back(p.second);
    }
    return result;
}();

std::vector<int> g_unique_values = []() {
    std::set<int> temp(g_data.begin(), g_data.end());
    return std::vector<int>(temp.begin(), temp.end());
}();


static void BM_MultisetRangeConstruct(benchmark::State& state) {
    for (auto _ : state) {
        std::unordered_multiset<int> mm(g_data.begin(), g_data.end());
        benchmark::DoNotOptimize(mm);
        state.PauseTiming();
        mm.clear();
        state.ResumeTiming();
    }
}
BENCHMARK(BM_MultisetRangeConstruct);

static void BM_MultisetInsert(benchmark::State& state) {
    std::unordered_multiset<int> mm;
    for (auto _ : state) {
        for (int i : g_data) {
            mm.insert(i);
        }
        benchmark::DoNotOptimize(mm);
        state.PauseTiming();
        assert(mm.size() == g_data.size());
        mm.clear();
        state.ResumeTiming();
    }
}
BENCHMARK(BM_MultisetInsert);

static void BM_MultisetErase(benchmark::State& state) {
    std::unordered_multiset<int> mm;
    for (auto _ : state) {
        state.PauseTiming();
        mm.insert(g_data.begin(), g_data.end());
        state.ResumeTiming();
        for (int i : g_unique_values) {
            mm.erase(i);
        }
        benchmark::DoNotOptimize(mm);
        assert(mm.empty());
    }
}
BENCHMARK(BM_MultisetErase);

static void BM_MultisetCountPresent(benchmark::State& state) {
    std::unordered_multiset<int> mm(g_data.begin(), g_data.end());
    for (auto _ : state) {
        for (int i : g_unique_values) {
            int c = mm.count(i);  // definitely present
            benchmark::DoNotOptimize(c);
        }
    }
}
BENCHMARK(BM_MultisetCountPresent);

static void BM_MultisetCountAbsent(benchmark::State& state) {
    std::unordered_multiset<int> mm(g_data.begin(), g_data.end());
    for (auto _ : state) {
        for (int i : g_unique_values) {
            int c = mm.count(i + 1);  // probably absent
            benchmark::DoNotOptimize(c);
        }
    }
}
BENCHMARK(BM_MultisetCountAbsent);

BENCHMARK_MAIN();
