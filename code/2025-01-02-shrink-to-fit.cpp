#include <cstdio>
#include <deque>
#include <string>
#include <vector>

using ValueType = char;
int g_allocated = 0;

template<class T>
struct A {
    using value_type = T;
    friend bool operator==(A, A) { return true; }
    explicit A() = default;
    template<class U> A(A<U>) {}
    static T *allocate(size_t n) {
        if constexpr (std::is_same_v<T, ValueType>) {
            g_allocated += n;
        }
        return std::allocator<T>().allocate(n);
    }
    static void deallocate(T *p, size_t n) {
        if constexpr (std::is_same_v<T, ValueType>) {
            g_allocated -= n;
        }
        std::allocator<T>().deallocate(p, n);
    }
};

size_t capacity_of(const std::vector<ValueType>& v) { return v.capacity(); }
size_t capacity_of(const std::string& v) { return v.capacity(); }
size_t capacity_of(const std::deque<ValueType, A<ValueType>>& v) { return g_allocated; }

template<class V>
void shrink_to_fit(V& v) {
  v = {1,2,3};
  printf("Before shrink_to_fit(): %zu\n", capacity_of(v));
  v.shrink_to_fit();
  printf("After shrink_to_fit(): %zu\n", capacity_of(v));
}

template<class V>
void clear(V& v) {
  printf("Before clear: %zu\n", capacity_of(v));
  v.clear();
  printf("After clear: %zu\n", capacity_of(v));
}

template<class V>
void resize(V& v) {
  printf("Before resize(0): %zu\n", capacity_of(v));
  v.resize(0);
  printf("After resize(0): %zu\n", capacity_of(v));
}

template<class V>
void assign_braces(V& v) {
  printf("Before ={}: %zu\n", capacity_of(v));
  v = {};
  printf("After ={}: %zu\n", capacity_of(v));
}

template<class V>
void move_assign(V& v) {
  printf("Before =V(): %zu\n", capacity_of(v));
  v = V();
  printf("After =V(): %zu\n", capacity_of(v));
}

template<class V>
void swap(V& v) {
  printf("Before V().swap: %zu\n", capacity_of(v));
  V().swap(v);
  printf("After V().swap: %zu\n", capacity_of(v));
}

int main() {
  {
    puts("std::vector ---");
    using V = std::vector<ValueType>;
    {
      V v = V(10000, 42);
      v.resize(0);
      shrink_to_fit(v);
    }
    {
      V v = {1,2,3};
      v.reserve(100);
      clear(v);
    }
    {
      V v = {1,2,3};
      v.reserve(100);
      resize(v);
    }
    {
      V v = {1,2,3};
      v.reserve(100);
      assign_braces(v);
    }
    {
      V v = {1,2,3};
      v.reserve(100);
      move_assign(v);
    }
    {
      V v = {1,2,3};
      v.reserve(100);
      swap(v);
    }
  }
  {
    puts("\nstd::string ---");
    using V = std::string;
    {
      V v = V(10000, 42);
      v.resize(0);
      shrink_to_fit(v);
    }
    {
      V v = {1,2,3};
      v.reserve(100);
      clear(v);
    }
    {
      V v = {1,2,3};
      v.reserve(100);
      resize(v);
    }
    {
      V v = {1,2,3};
      v.reserve(100);
      assign_braces(v);
    }
    {
      V v = {1,2,3};
      v.reserve(100);
      move_assign(v);
    }
    {
      V v = {1,2,3};
      v.reserve(100);
      swap(v);
    }
  }
  {
    puts("\nstd::deque ---");
    using V = std::deque<ValueType, A<ValueType>>;
    {
      V v;
      printf("Default-constructed capacity: %zu\n", capacity_of(v));
      v = {1};
      printf("Capacity after ={1}: %zu\n---\n", capacity_of(v));
    }
    {
      V v = V(10000, 42);
      v.resize(0);
      shrink_to_fit(v);
    }
    {
      V v = V(10000, 42);
      clear(v);
    }
    {
      V v = V(10000, 42);
      resize(v);
    }
    {
      V v = V(10000, 42);
      assign_braces(v);
    }
    {
      V v = V(10000, 42);
      move_assign(v);
    }
    {
      V v = V(10000, 42);
      swap(v);
    }
  }
}