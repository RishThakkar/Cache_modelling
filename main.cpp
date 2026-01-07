#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "cache_model.h"

static void run_sequential(Cache& cache, uint64_t bytes, uint64_t step_bytes) {
    uint64_t lat = 0;
    for (uint64_t addr = 0; addr < bytes; addr += step_bytes) {
        cache.access(addr, lat);
    }
}

static void run_stride(Cache& cache, uint64_t working_set_bytes, uint64_t stride_bytes, uint64_t accesses) {
    uint64_t lat = 0;
    uint64_t addr = 0;
    for (uint64_t i = 0; i < accesses; i++) {
        cache.access(addr, lat);
        addr = (addr + stride_bytes) % working_set_bytes;
    }
}

static void run_working_set(Cache& cache, uint64_t working_set_bytes, uint64_t step_bytes, uint64_t passes) {
    uint64_t lat = 0;
    for (uint64_t p = 0; p < passes; p++) {
        for (uint64_t addr = 0; addr < working_set_bytes; addr += step_bytes) {
            cache.access(addr, lat);
        }
    }
}

int main() {
    const size_t hit_latency = 1;
    const size_t miss_penalty = 100;

    std::ofstream out("results.csv");
    out << "experiment,cache_kb,line_size,assoc,stride_bytes,working_set_kb,miss_rate,amat,hits,misses\n";

    // 1) Miss rate vs cache size
    for (size_t cache_kb : {4, 8, 16, 32, 64, 128}) {
        Cache c(cache_kb * 1024, 64, 4, hit_latency, miss_penalty);
        c.reset_stats();
        run_sequential(c, 1ull << 20, 4);

        out << "miss_vs_size" << "," << cache_kb << "," << 64 << "," << 4
            << "," << 0 << "," << 0
            << "," << c.get_miss_rate() << "," << c.get_amat()
            << "," << c.get_hits() << "," << c.get_misses() << "\n";
    }

    // 2) Miss rate vs associativity
    for (size_t assoc : {1, 2, 4, 8, 16}) {
        Cache c(32 * 1024, 64, assoc, hit_latency, miss_penalty);
        c.reset_stats();
        run_sequential(c, 1ull << 20, 4);

        out << "miss_vs_assoc" << "," << 32 << "," << 64 << "," << assoc
            << "," << 0 << "," << 0
            << "," << c.get_miss_rate() << "," << c.get_amat()
            << "," << c.get_hits() << "," << c.get_misses() << "\n";
    }

    // 3) AMAT vs line size
    for (size_t line_sz : {16, 32, 64, 128, 256}) {
        Cache c(32 * 1024, line_sz, 4, hit_latency, miss_penalty);
        c.reset_stats();
        run_sequential(c, 1ull << 20, 4);

        out << "amat_vs_line" << "," << 32 << "," << line_sz << "," << 4
            << "," << 0 << "," << 0
            << "," << c.get_miss_rate() << "," << c.get_amat()
            << "," << c.get_hits() << "," << c.get_misses() << "\n";
    }

    // 4) Conflict misses: stride patterns (direct-mapped makes it obvious)
    for (size_t stride : {64, 128, 256, 512, 1024, 2048}) {
        Cache c(32 * 1024, 64, 1, hit_latency, miss_penalty);
        c.reset_stats();
        run_stride(c, /*working_set=*/32 * 1024, /*stride=*/stride, /*accesses=*/200000);

        out << "conflict_stride" << "," << 32 << "," << 64 << "," << 1
            << "," << stride << "," << 32
            << "," << c.get_miss_rate() << "," << c.get_amat()
            << "," << c.get_hits() << "," << c.get_misses() << "\n";
    }

    // 5) Capacity misses: working set sweep
    for (size_t ws_kb : {4, 8, 16, 32, 64, 128, 256}) {
        Cache c(32 * 1024, 64, 4, hit_latency, miss_penalty);
        c.reset_stats();
        run_working_set(c, ws_kb * 1024, 64, /*passes=*/20);

        out << "capacity_workingset" << "," << 32 << "," << 64 << "," << 4
            << "," << 0 << "," << ws_kb
            << "," << c.get_miss_rate() << "," << c.get_amat()
            << "," << c.get_hits() << "," << c.get_misses() << "\n";
    }

    std::cout << "Wrote results.csv\n";
    return 0;
}
