#include <iostream>
#include <x86intrin.h>
#include <vector>
#include <cstdint>
#include <atomic>
#include <immintrin.h>

// lock cpu frequency before benchmarking (Google benchmark does this uint64_tmatically) (Clock speed ramp up - Program starts at low clock speed)
// Pin the program to a single core (taskset or sched_setaffinity)
// Ensure program is not interrupted mid run (kernel boot params, lscpu, nohz_full, rcu_nocbs = 12)
// Ensure nothing else runs on that core (also disable SMT)
//
// still not consistent benchmark?? Maybe because of compiler instruction reordering or CPU out-of-order execution
// Use memory_fences

#include <unistd.h>
#include <sched.h>
bool pin_to_core(int core) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);

    return sched_setaffinity(
        0, sizeof(cpu_set_t), &cpuset
    ) == 0;
}

void pollute_cache() {
    constexpr size_t CACHE_LINES = (512ULL * 1024 * 1024) / 64;
    static std::vector<uint64_t> data(CACHE_LINES * 8);

    for (size_t i = 0; i < data.size(); i += 8) {
        data[i]++;
    }
}

long A(bool flag) {
  volatile int* x = new int(4);
  // Why volatile?? int is small so cpu might just keep it in the register. Faster than even cache accessa
  if(flag) {
    _mm_mfence();
    pollute_cache();
  }
  _mm_mfence();
  uint64_t start = __rdtsc();
  int y = *x+5;
  _mm_lfence(); // very important. Prevents OOO
  uint64_t end = __rdtsc();
  uint64_t time = end-start;

  return time;
}

int main() {
  pin_to_core(12);

  std::cout << "No pollution : " << A(false) << " ticks\n";
  std::cout << "With pollution : " << A(true) << " ticks\n";

  // Does order matter??
}
