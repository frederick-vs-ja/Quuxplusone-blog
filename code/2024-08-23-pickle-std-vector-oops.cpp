#include <boost/interprocess/managed_shared_memory.hpp>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <vector>

// DANGER, WILL ROBINSON!
// This code will not work as written, because it stores a `std::vector<int>`
// into the shared memory segment. When reloaded at a different virtual address,
// all the raw pointers in the data members of the `vector` object will still
// hold their original values, meaning that they point to what are now the
// wrong addresses. Also, the vector's heap-allocated array of ints will just
// be gone, anyway, because it was allocated from the first process's global heap.
//

// This is the name of the shared memory segment Boost will create for us.
// On Posix systems, it's likely a disk file named "/tmp/boost_interprocess/PICKLE".
#define SHM_SEGMENT_NAME "PICKLE"

namespace bip = boost::interprocess;

int main(int argc, char **argv) {
  if (argc >= 3 && std::string_view(argv[1]) == "--pickle") {
    bip::shared_memory_object::remove(SHM_SEGMENT_NAME);
    auto segment = bip::managed_shared_memory(bip::create_only, SHM_SEGMENT_NAME, 10'000);
    std::vector<int> *p = segment.construct<std::vector<int>>("MY_INTS")();
    for (int i = 2; i < argc; ++i) {
      p->push_back(atoi(argv[i]));
    }
    printf("Wrote %zu elements to a vector in the shared memory segment (vector at %p, data at %p)\n", p->size(), p, p->data());
  } else if (argc == 2 && std::string_view(argv[1]) == "--unpickle") {
    auto segment = bip::managed_shared_memory(bip::open_only, SHM_SEGMENT_NAME);
    auto [p, nelem] = segment.find<std::vector<int>>("MY_INTS");
    assert(p != nullptr); // it was found
    assert(nelem == 1);   // and it's a single vector<int> object
    printf("Found a vector with %zu elements in the shared memory segment (vector at %p, data at %p)\n", p->size(), p, p->data());
    printf("Those elements are:\n");
    for (int i : *p) {
      printf("  %d\n", i);
    }
  } else {
    puts("Usage:");
    puts("  ./a.out --pickle 1 2 3");
    puts("  ./a.out --unpickle");
  }
}
