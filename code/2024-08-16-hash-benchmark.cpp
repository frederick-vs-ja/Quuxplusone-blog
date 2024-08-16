#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>

size_t g_allocated = 0;

template<class T>
struct A {
  using value_type = T;
  explicit A() = default;
  template<class U> A(const A<U>&) {}
  static T *allocate(size_t n) {
    g_allocated += n*sizeof(T);
    return std::allocator<T>().allocate(n);
  }
  static void deallocate(T *p, size_t n) {
    g_allocated -= n*sizeof(T);
    std::allocator<T>().deallocate(p, n);
  }
};

template<class T> std::vector<T> factory();

template<>
std::vector<int*> factory<int*>() {
  int *p = new int[100'000];
  std::vector<int*> v;
  for (int i=0; i < 100'000; ++i) v.push_back(&p[i]);
  std::mt19937 g;
  std::shuffle(v.begin(), v.end(), g);
  return v;
}

template<>
std::vector<std::string> factory<std::string>() {
  std::mt19937 g;
  std::vector<std::string> v;
  auto ab = std::string(10'000, 'x');
  for (auto&& c : ab) c = (g() % 26) + 'A';
  for (int i=0; i < 100'000; ++i) {
    std::shuffle(ab.begin(), ab.end(), g);
    v.push_back(ab);
  }
  return v;
}

template<>
std::vector<std::vector<bool>> factory<std::vector<bool>>() {
  std::mt19937 g;
  std::vector<std::vector<bool>> v;
  auto ab = std::array<bool, 80'000>();
  for (auto&& c : ab) c = (g() % 2);
  for (int i=0; i < 100'000; ++i) {
    std::shuffle(ab.begin(), ab.end(), g);
    v.emplace_back(ab.begin(), ab.end());
  }
  return v;
}

template<class T, int Which>
void test(std::vector<T> v) {
  struct H1 { size_t operator()(const T& t) const { return std::hash<T>()(t); } };
  struct H2 { size_t operator()(const T& t) const noexcept { return std::hash<T>()(t); } };

  using Hash = std::conditional_t<
    (Which == 0), H1, std::conditional_t<
      (Which == 1), H2, std::hash<T>
    >
  >;
  const char *desc =
    (Which == 0) ? "non-noexcept" :
    (Which == 1) ? "noexcept" : "std::hash";

  {
    std::unordered_set<T, Hash, std::equal_to<T>, A<T>> s = {T()};
    s.clear();
    g_allocated = 0;
    s.insert(T());
    size_t node_size = std::exchange(g_allocated, 0);
    printf(" node size when hasher is %s: %zu\n", desc, node_size);

    std::unordered_set<T, Hash, std::equal_to<T>, A<T>> s2;
    size_t rehashes = 0;
    auto start = std::chrono::system_clock::now();
    for (T& t : v) {
      s2.insert(std::move(t));
      rehashes += (std::exchange(g_allocated, 0) != node_size);
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
    printf("  Time to do %zu rehashes: %zu ms\n", rehashes, size_t(elapsed.count()));
  }
}

int main() {
  if (auto v = factory<int*>(); true) {
    puts("With int*:");
    test<int*, 0>(v);
    test<int*, 1>(v);
    test<int*, 2>(v);
  }
  if (auto v = factory<std::string>(); true) {
    puts("With string:");
    test<std::string, 0>(v);
    test<std::string, 1>(v);
    test<std::string, 2>(v);
  }
  if (auto v = factory<std::vector<bool>>(); true) {
    puts("With vector<bool>:");
    test<std::vector<bool>, 0>(v);
    test<std::vector<bool>, 1>(v);
    test<std::vector<bool>, 2>(v);
  }
}
