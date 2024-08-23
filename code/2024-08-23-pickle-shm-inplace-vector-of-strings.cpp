#include <boost/container/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <scoped_allocator>
#include <sg14/aa_inplace_vector.h>
#include <string_view>
#include <vector>

// This is the name of the shared memory segment Boost will create for us.
// On Posix systems, it's likely a disk file named "/tmp/boost_interprocess/PICKLE".
#define SHM_SEGMENT_NAME "PICKLE"

namespace bip = boost::interprocess;

namespace shm {
  template<class T> using allocator = std::scoped_allocator_adaptor<bip::allocator<T, bip::managed_shared_memory::segment_manager>>;
  template<class T> using vector = std::vector<T, shm::allocator<T>>;
  template<class T, size_t N> using inplace_vector = sg14::inplace_vector<T, N, shm::allocator<T>>;
  using string = boost::container::basic_string<char, std::char_traits<char>, shm::allocator<char>>;
}

using VectorOfStrings = shm::inplace_vector<shm::string, 10>;

int main(int argc, char **argv) {
  if (argc >= 3 && std::string_view(argv[1]) == "--pickle") {
    bip::shared_memory_object::remove(SHM_SEGMENT_NAME);
    auto segment = bip::managed_shared_memory(bip::create_only, SHM_SEGMENT_NAME, 10'000);
    auto alloc = shm::allocator<int>(segment.get_segment_manager());
    auto *p = segment.construct<VectorOfStrings>("MY_STRINGS")(argv+2, argv+argc, alloc);
    printf("Wrote %zu elements to a vector in the shared memory segment (vector at %p, data at %p)\n", p->size(), p, p->data());
  } else if (argc == 2 && std::string_view(argv[1]) == "--unpickle") {
    auto segment = bip::managed_shared_memory(bip::open_only, SHM_SEGMENT_NAME);
    auto [p, nelem] = segment.find<VectorOfStrings>("MY_STRINGS");
    assert(p != nullptr); // it was found
    assert(nelem == 1);   // and it's a single object, not an array
    printf("Found a vector with %zu elements in the shared memory segment (vector at %p, data at %p)\n", p->size(), p, p->data());
    printf("Those elements are:\n");
    for (shm::string& s : *p) {
      printf("  %s\n", s.c_str());
    }
  } else {
    puts("Usage:");
    puts("  ./a.out --pickle abc def ghi");
    puts("  ./a.out --unpickle");
  }
}
