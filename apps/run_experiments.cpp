#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

#include "cache/cache_model.h"
#include "cache/replacement_policy.h"

// -----------------------------
// Trace patterns (simple + reusable)
// -----------------------------

// 1) Streaming sequential: strong spatial locality, almost no temporal reuse
// Miss rate is mostly ~ step / line_size (cold, single pass)
static void trace_stream_sequential(Cache& cache, uint64_t bytes, uint64_t step_bytes) {
    uint64_t lat = 0;
    for (uint64_t addr = 0; addr < bytes; addr += step_bytes) {
        cache.access(addr, lat);
    }
}

// 2) Reuse working set: shows capacity effects clearly (cache size matters)
// Multiple passes over a fixed working set -> after warm-up, if WS fits, misses drop a lot.
static void trace_reuse_working_set(Cache& cache, uint64_t working_set_bytes, uint64_t step_bytes, uint64_t passes) {
    uint64_t lat = 0;
    for (uint64_t p = 0; p < passes; p++) {
        for (uint64_t addr = 0; addr < working_set_bytes; addr += step_bytes) {
            cache.access(addr, lat);
        }
    }
}

// 3) Conflict pattern: demonstrates associativity benefits
// Touch (hot_lines) addresses that map to the same set by spacing them cache_size apart.
// For direct-mapped or low associativity this can thrash.
static void trace_same_set_conflict(Cache& cache,
                                   uint64_t cache_size_bytes,
                                   uint64_t hot_lines,
                                   uint64_t accesses) {
    uint64_t lat = 0;

    // Addresses separated by cache_size_bytes map to same index (same set) for typical index function.
    std::vector<uint64_t> addrs;
    addrs.reserve(hot_lines);
    for (uint64_t i = 0; i < hot_lines; i++) {
        addrs.push_back(i * cache_size_bytes);
    }

    for (uint64_t i = 0; i < accesses; i++) {
        cache.access(addrs[i % addrs.size()], lat);
    }
}

// Helper: stringify policy for CSV
static const char* policy_name(ReplacementPolicy p) {
    switch (p) {
        case ReplacementPolicy::LRU:    return "LRU";
        case ReplacementPolicy::FIFO:   return "FIFO";
        case ReplacementPolicy::RANDOM: return "RANDOM";
        default:                        return "UNKNOWN";
    }
}

// Write one CSV row
static void write_row(std::ofstream& out,
                      const std::string& experiment,
                      size_t cache_kb,
                      size_t line_size,
                      size_t assoc,
                      ReplacementPolicy pol,
                      const std::string& trace_name,
                      uint64_t working_set_kb,
                      uint64_t stride_bytes,
                      double miss_rate,
                      double amat,
                      uint64_t hits,
                      uint64_t misses) {
    out << experiment << ","
        << cache_kb << ","
        << line_size << ","
        << assoc << ","
        << policy_name(pol) << ","
        << trace_name << ","
        << working_set_kb << ","
        << stride_bytes << ","
        << std::fixed << std::setprecision(6) << miss_rate << ","
        << std::fixed << std::setprecision(3) << amat << ","
        << hits << ","
        << misses
        << "\n";
}

int main() {
    // Timing model (simple)
    const size_t hit_latency = 1;
    const size_t miss_penalty = 100;

    std::ofstream out("results.csv");
    out << "experiment,cache_kb,line_size,assoc,policy,trace,working_set_kb,stride_bytes,miss_rate,amat,hits,misses\n";

    // -----------------------------
    // Baseline configuration
    // We'll change ONE parameter at a time from this baseline.
    // -----------------------------
    const size_t base_cache_kb = 32;
    const size_t base_line_size = 64;
    const size_t base_assoc = 4;
    const ReplacementPolicy base_policy = ReplacementPolicy::LRU;

    // Traces (kept simple and stable)
    const uint64_t stream_bytes = 1ull << 20;         // 1MB stream
    const uint64_t stream_step = 4;                   // word step
    const uint64_t reuse_ws_kb = 24;                  // < 32KB so it can fit in baseline cache
    const uint64_t reuse_passes = 50;
    const uint64_t conflict_accesses = 200000;

    // -----------------------------
    // 0) Baseline run (so plots have a reference point)
    // -----------------------------
    {
        Cache c(base_cache_kb * 1024, base_line_size, base_assoc, hit_latency, miss_penalty, base_policy);
        c.reset_stats();
        trace_reuse_working_set(c, reuse_ws_kb * 1024, stream_step, reuse_passes);

        write_row(out, "baseline",
                  base_cache_kb, base_line_size, base_assoc, base_policy,
                  "reuse_working_set",
                  reuse_ws_kb, 0,
                  c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
    }

    // -----------------------------
    // 1) Change ONLY cache size (capacity effect)
    // Keep line size, assoc, policy fixed.
    // Use reuse_working_set trace so cache size actually matters.
    // -----------------------------
    for (size_t cache_kb : {4, 8, 16, 24, 32, 48, 64, 96, 128}) {
        Cache c(cache_kb * 1024, base_line_size, base_assoc, hit_latency, miss_penalty, base_policy);
        c.reset_stats();
        trace_reuse_working_set(c, reuse_ws_kb * 1024, stream_step, reuse_passes);

        write_row(out, "sweep_cache_size",
                  cache_kb, base_line_size, base_assoc, base_policy,
                  "reuse_working_set",
                  reuse_ws_kb, 0,
                  c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
    }

    // -----------------------------
    // 2) Change ONLY associativity (conflict effect)
    // Keep cache size, line size, policy fixed.
    // Use a same-set conflict trace to make associativity show up clearly.
    // We set hot_lines = assoc + 1 to force thrash when assoc is too small.
    // -----------------------------
    for (size_t assoc : {1, 2, 4, 8, 16}) {
        Cache c(base_cache_kb * 1024, base_line_size, assoc, hit_latency, miss_penalty, base_policy);
        c.reset_stats();

        const uint64_t hot_lines = static_cast<uint64_t>(assoc) + 1; // slightly more than ways
        trace_same_set_conflict(c, base_cache_kb * 1024, hot_lines, conflict_accesses);

        write_row(out, "sweep_associativity",
                  base_cache_kb, base_line_size, assoc, base_policy,
                  "same_set_conflict",
                  0, 0,
                  c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
    }

    // -----------------------------
    // 3) Change ONLY line size (spatial locality effect)
    // Keep cache size, assoc, policy fixed.
    // Use streaming sequential so line size impacts miss rate strongly.
    // -----------------------------
    for (size_t line_sz : {16, 32, 64, 128, 256}) {
        Cache c(base_cache_kb * 1024, line_sz, base_assoc, hit_latency, miss_penalty, base_policy);
        c.reset_stats();
        trace_stream_sequential(c, stream_bytes, stream_step);

        write_row(out, "sweep_line_size",
                  base_cache_kb, line_sz, base_assoc, base_policy,
                  "stream_sequential",
                  0, 0,
                  c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
    }

    // -----------------------------
    // 4) Change ONLY replacement policy
    // Keep cache size, line size, assoc fixed.
    // Use a conflict-ish pattern where policy differences can show up.
    // (assoc+1 hot lines cycling inside one set)
    // -----------------------------
    for (ReplacementPolicy pol : {ReplacementPolicy::LRU, ReplacementPolicy::FIFO, ReplacementPolicy::RANDOM}) {
        Cache c(base_cache_kb * 1024, base_line_size, base_assoc, hit_latency, miss_penalty, pol);
        c.reset_stats();

        const uint64_t hot_lines = static_cast<uint64_t>(base_assoc) + 1;
        trace_same_set_conflict(c, base_cache_kb * 1024, hot_lines, conflict_accesses);

        write_row(out, "sweep_policy",
                  base_cache_kb, base_line_size, base_assoc, pol,
                  "same_set_conflict",
                  0, 0,
                  c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
    }

    std::cout << "Wrote results.csv\n";
    return 0;
}
