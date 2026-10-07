#include <iostream>
#include <chrono>
#include <vector>
#include <unordered_map>

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

int NUM_ELEMENTS = 1024;

struct alignas(2048) int2048 { // small values dont work because of hardware prefetching
  int a;
};

struct int4 {
  int a;
};

template <typename A, typename B>
void benchmark(std::vector<A>& vec, std::unordered_map<int,B>& hashmap) {
	volatile int sum = 0; long firstns;
	auto start = std::chrono::steady_clock::now();
	for(int i=0; i<NUM_ELEMENTS; i++) {
		sum += vec[i].a;
		if(i==0) {
			auto first = std::chrono::steady_clock::now();
			firstns = std::chrono::duration_cast<std::chrono::nanoseconds>(first-start).count();
		}
	}
	auto end = std::chrono::steady_clock::now();
	auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();

    std::cout << "Vector first access : " << firstns << " ns\n";
    std::cout << "Vector full access : " << ns << " ns\n";

	sum = 0;
	start = std::chrono::steady_clock::now();
	for(int i=0; i<NUM_ELEMENTS; i++) {
	    sum += hashmap[i].a;
		if(i==0) {
			auto first = std::chrono::steady_clock::now();
			firstns = std::chrono::duration_cast<std::chrono::nanoseconds>(first-start).count();
		}
	}
	end = std::chrono::steady_clock::now();
	ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();

    std::cout << "Hashmap first access : " << firstns << " ns\n";
    std::cout << "Hashmap full access : " << ns << " ns\n";
}

int main() {
  pin_to_core(12);

  std::vector<int4> vec4; std::unordered_map<int,int4> hashmap4;
  std::vector<int2048> vec2048; std::unordered_map<int, int2048> hashmap2048;
  for(int i=0; i<NUM_ELEMENTS; i++) {
    vec4.push_back({i});
    hashmap4[i] = {i*10};

    vec2048.push_back({i});
    hashmap2048[i] = {i*10};
  }

  benchmark(vec4, hashmap4); // 37.28% cache miss rate (34,000 misses)
  std::cout << "\n\nnow 2048 bytes aligned vector \n";
  benchmark(vec2048, hashmap2048); // 64.79% cache miss rate (220,000 misses)

}

// can also show total cpu_cycles with compiler optimisations enabled/disabled
