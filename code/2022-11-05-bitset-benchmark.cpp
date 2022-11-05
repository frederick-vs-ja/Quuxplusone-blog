
#define _LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION 1
#include <algorithm>
#include <benchmark/benchmark.h>
#include <bitset>
#include <boost/dynamic_bitset.hpp>
#include <type_traits>
#include <vector>

constexpr size_t N = 1024*1024;
constexpr size_t OFFSET = 435;

template<class T>
T make()
{
    T x = T();
    if constexpr (!std::is_same_v<T, std::bitset<N>>) {
        x.resize(N);
    }
    for (size_t i=0; i < N; i += 6565 + i % 20) {
        x[i] = true;
    }
    return x;
}

template<class T>
int generic_manual(int n, const T& x) {
    for (int i = n; i < N; ++i) {
        if (x[i]) return i;
    }
    return N;
}

static int vectorbool_stdfind(int n, const std::vector<bool>& x) {
    if (n >= N) return N;
    return std::find(x.begin() + n, x.end(), true) - x.begin();
}

#ifdef __GLIBCXX__
static int bitset_findnext(int n, const std::bitset<N>& x) {
    return (n == 0) ? x._Find_first() : x._Find_next(n-1);
}
#endif

static int dynamic_bitset_findnext(int n, const boost::dynamic_bitset<>& x) {
    if (n >= N) return N;
    return (n == 0) ? x.find_first() : x.find_next(n-1);
}

// =======================================================================

#define LIMIT_ITERATIONS ->Iterations(100000)
#if __GLIBCXX__
 #define LIMIT_ITERATIONS_ON_LIBSTDCXX ->Iterations(100000)
#else
 #define LIMIT_ITERATIONS_ON_LIBSTDCXX
#endif

static void VectorBoolManual(benchmark::State &state) {
  auto x = make<std::vector<bool>>();
  size_t pos = 1;
  for (auto _ : state) {
    pos = (pos + OFFSET) % N;
    benchmark::DoNotOptimize(generic_manual(pos, x));
  }
}
BENCHMARK(VectorBoolManual) LIMIT_ITERATIONS;

static void VectorBoolStdFind(benchmark::State &state) {
  auto x = make<std::vector<bool>>();
  size_t pos = 1;
  for (auto _ : state) {
    pos = (pos + OFFSET) % N;
    benchmark::DoNotOptimize(vectorbool_stdfind(pos, x));
  }
}
BENCHMARK(VectorBoolStdFind) LIMIT_ITERATIONS_ON_LIBSTDCXX;

static void BitsetManual(benchmark::State &state) {
  auto x = make<std::bitset<N>>();
  size_t pos = 1;
  for (auto _ : state) {
    pos = (pos + OFFSET) % N;
    benchmark::DoNotOptimize(generic_manual(pos, x));
  }
}
BENCHMARK(BitsetManual) LIMIT_ITERATIONS;

#if __GLIBCXX__
static void BitsetFindnext(benchmark::State &state) {
  auto x = make<std::bitset<N>>();
  size_t pos = 1;
  for (auto _ : state) {
    pos = (pos + OFFSET) % N;
    benchmark::DoNotOptimize(bitset_findnext(pos, x));
  }
}
BENCHMARK(BitsetFindnext);
#endif

static void DynBitsetManual(benchmark::State &state) {
  auto x = make<boost::dynamic_bitset<>>();
  size_t pos = 1;
  for (auto _ : state) {
    pos = (pos + OFFSET) % N;
    benchmark::DoNotOptimize(generic_manual(pos, x));
  }
}
BENCHMARK(DynBitsetManual);

static void DynBitsetFindnext(benchmark::State &state) {
  auto x = make<boost::dynamic_bitset<>>();
  size_t pos = 1;
  for (auto _ : state) {
    pos = (pos + OFFSET) % N;
    benchmark::DoNotOptimize(dynamic_bitset_findnext(pos, x));
  }
}
BENCHMARK(DynBitsetFindnext);

BENCHMARK_MAIN();
