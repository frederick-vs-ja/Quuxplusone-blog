#include <boost/interprocess/managed_shared_memory.hpp>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string_view>

// This is the name of the shared memory segment Boost will create for us.
// On Posix systems, it's likely a disk file named "/tmp/boost_interprocess/PICKLE".
#define SHM_SEGMENT_NAME "PICKLE"

namespace bip = boost::interprocess;

int main(int argc, char **argv) {
  if (argc == 3 && std::string_view(argv[1]) == "--pickle") {
    bip::shared_memory_object::remove(SHM_SEGMENT_NAME);
    auto segment = bip::managed_shared_memory(bip::create_only, SHM_SEGMENT_NAME, 10'000);
    int *p = segment.construct<int>("MY_INT")();
    *p = atoi(argv[2]);
    printf("Wrote %d to the shared memory segment (at RAM address %p)\n", *p, p);
  } else if (argc == 2 && std::string_view(argv[1]) == "--unpickle") {
    auto segment = bip::managed_shared_memory(bip::open_only, SHM_SEGMENT_NAME);
    auto [p, nelem] = segment.find<int>("MY_INT");
    assert(p != nullptr); // it was found
    assert(nelem == 1);   // and it's a single int object, not an array
    printf("Read %d from the shared memory segment (at RAM address %p)\n", *p, p);
  } else {
    puts("Usage:");
    puts("  ./a.out --pickle 42");
    puts("  ./a.out --unpickle");
  }
}
