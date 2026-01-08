#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

#include "cache/cache_model.h"
#include "cache/replacement_policy.h"

// -----------------------------
// Trace patterns
// -----------------------------

// Streaming sequential: strong spatial locality, minimal temporal reuse.
static void trace_stream_sequential(Cache& cache, uint64_t bytes, uint64_t step_bytes) {
    uint64_t lat = 0;
    for (uint64_t addr = 0; addr < bytes; addr += step_bytes) {
        cache.access(addr, lat);
    }
}

// Reuse working set: shows capacity effects (cache size matters).
static void trace_reuse_working_set(Cache& cache, uint64_t working_set_bytes, uint64_t step_bytes, uint64_t passes) {
    uint64_t lat = 0;
    for (uint64_t p = 0; p < passes; p++) {
        for (uint64_t addr = 0; addr < working_set_bytes; addr += step_bytes) {
            cache.access(addr, lat);
        }
    }
}

// Same-set conflict: demonstrates associativity/policy differences.
// Addresses spaced cache_size apart map to same set for typical indexing.
static void trace_same_set_conflict(Cache& cache, uint64_t cache_size_bytes, uint64_t hot_lines, uint64_t accesses) {
    uint64_t lat = 0;
    std::vector<uint64_t> addrs;
    addrs.reserve(hot_lines);
    for (uint64_t i = 0; i < hot_lines; i++) {
        addrs.push_back(i * cache_size_bytes);
    }

    for (uint64_t i = 0; i < accesses; i++) {
        cache.access(addrs[i % addrs.size()], lat);
    }
}

// Stride walk within a working set: can show spatial locality effects + set conflicts depending on stride.
static void trace_stride(Cache& cache, uint64_t working_set_bytes, uint64_t stride_bytes, uint64_t accesses) {
    uint64_t lat = 0;
    uint64_t addr = 0;
    for (uint64_t i = 0; i < accesses; i++) {
        cache.access(addr, lat);
        addr = (addr + stride_bytes) % working_set_bytes;
    }
}

// -----------------------------
// Helpers
// -----------------------------

static const char* policy_name(ReplacementPolicy p) {
    switch (p) {
        case ReplacementPolicy::LRU:    return "LRU";
        case ReplacementPolicy::FIFO:   return "FIFO";
        case ReplacementPolicy::RANDOM: return "RANDOM";
        default:                        return "UNKNOWN";
    }
}

static void write_row(std::ofstream& out,
                      const std::string& experiment,
                      size_t cache_kb,
                      size_t line_size,
                      size_t assoc,
                      size_t hit_latency,
                      size_t miss_penalty,
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
        << hit_latency << ","
        << miss_penalty << ","
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
    // -----------------------------
    // Baseline configuration
    // -----------------------------
    const size_t base_cache_kb   = 32;
    const size_t base_line_size  = 64;
    const size_t base_assoc      = 4;
    const size_t base_hit_lat    = 1;
    const size_t base_miss_pen   = 100;
    const ReplacementPolicy base_policy = ReplacementPolicy::LRU;

    // Trace knobs (kept stable)
    const uint64_t stream_bytes   = 1ull << 20;  // 1MB
    const uint64_t step_word      = 4;
    const uint64_t reuse_passes   = 50;
    const uint64_t conflict_acc   = 200000;
    const uint64_t stride_acc     = 200000;

    MemoryTiming mem;
    mem.fixed_latency_cycles = 60;  // tune this
    mem.bytes_per_cycle = 16;       // 16B/cycle

    std::ofstream out("results.csv");
    out << "experiment,cache_kb,line_size,assoc,hit_latency,miss_penalty,policy,trace,working_set_kb,stride_bytes,miss_rate,amat,hits,misses\n";

    // -----------------------------
    // 0) Baseline run (reference point)
    // -----------------------------
    {
        const uint64_t reuse_ws_kb = 24;
        Cache c(base_cache_kb * 1024, base_line_size, base_assoc, base_hit_lat, base_miss_pen, base_policy);
        c.reset_stats();
        trace_reuse_working_set(c, reuse_ws_kb * 1024, step_word, reuse_passes);

        write_row(out, "baseline",
                  base_cache_kb, base_line_size, base_assoc, base_hit_lat, base_miss_pen,
                  base_policy, "reuse_working_set",
                  reuse_ws_kb, 0,
                  c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
    }

    // -----------------------------
    // 1) Sweep cache size (capacity effect) - reuse workload
    // -----------------------------
    {
        const uint64_t reuse_ws_kb = 24;
        for (size_t cache_kb : {4, 8, 16, 24, 32, 48, 64, 96, 128}) {
            Cache c(cache_kb * 1024, base_line_size, base_assoc, base_hit_lat, base_miss_pen, base_policy);
            c.reset_stats();
            trace_reuse_working_set(c, reuse_ws_kb * 1024, step_word, reuse_passes);

            write_row(out, "sweep_cache_size",
                      cache_kb, base_line_size, base_assoc, base_hit_lat, base_miss_pen,
                      base_policy, "reuse_working_set",
                      reuse_ws_kb, 0,
                      c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
        }
    }

    // -----------------------------
    // 2) Sweep associativity (conflict effect)
    // -----------------------------
    {
        for (size_t assoc : {1, 2, 4, 8, 16}) {
            Cache c(base_cache_kb * 1024, base_line_size, assoc, base_hit_lat, base_miss_pen, base_policy);
            c.reset_stats();

            const uint64_t hot_lines = static_cast<uint64_t>(assoc) + 1;
            trace_same_set_conflict(c, base_cache_kb * 1024, hot_lines, conflict_acc);

            write_row(out, "sweep_associativity",
                      base_cache_kb, base_line_size, assoc, base_hit_lat, base_miss_pen,
                      base_policy, "same_set_conflict",
                      0, 0,
                      c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
        }
    }

    // -----------------------------
    // 3) Sweep line size (spatial locality on streaming)
    // -----------------------------
    {
        for (size_t line_sz : {16, 32, 64, 128, 256}) {
            Cache c(base_cache_kb * 1024, line_sz, base_assoc, base_hit_lat, mem, base_policy);
            c.reset_stats();
            trace_stream_sequential(c, stream_bytes, step_word);

            write_row(out, "sweep_line_size",
                      base_cache_kb, line_sz, base_assoc, base_hit_lat, base_miss_pen,
                      base_policy, "stream_sequential",
                      0, 0,
                      c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
        }
    }

    // -----------------------------
    // 4) Sweep replacement policy (conflict-ish trace)
    // -----------------------------
    {
        for (ReplacementPolicy pol : {ReplacementPolicy::LRU, ReplacementPolicy::FIFO, ReplacementPolicy::RANDOM}) {
            Cache c(base_cache_kb * 1024, base_line_size, base_assoc, base_hit_lat, base_miss_pen, pol);
            c.reset_stats();

            const uint64_t hot_lines = static_cast<uint64_t>(base_assoc) + 1;
            trace_same_set_conflict(c, base_cache_kb * 1024, hot_lines, conflict_acc);

            write_row(out, "sweep_policy_conflict",
                      base_cache_kb, base_line_size, base_assoc, base_hit_lat, base_miss_pen,
                      pol, "same_set_conflict",
                      0, 0,
                      c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
        }
    }

    // -----------------------------
    // 5) Sweep working set size (capacity curve)
    // Keep cache fixed; change only working set.
    // -----------------------------
    {
        for (uint64_t ws_kb : {4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 64, 96, 128}) {
            Cache c(base_cache_kb * 1024, base_line_size, base_assoc, base_hit_lat, base_miss_pen, base_policy);
            c.reset_stats();
            trace_reuse_working_set(c, ws_kb * 1024, step_word, reuse_passes);

            write_row(out, "sweep_working_set",
                      base_cache_kb, base_line_size, base_assoc, base_hit_lat, base_miss_pen,
                      base_policy, "reuse_working_set",
                      ws_kb, 0,
                      c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
        }
    }

    // -----------------------------
    // 6) Sweep stride (stride effects within a fixed working set)
    // Keep cache + WS fixed; change only stride.
    // -----------------------------
    {
        const uint64_t ws_kb = 32; // same as cache size for interesting behavior
        for (uint64_t stride : {4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048}) {
            Cache c(base_cache_kb * 1024, base_line_size, 1 /*direct-mapped*/, base_hit_lat, base_miss_pen, base_policy);
            c.reset_stats();
            trace_stride(c, ws_kb * 1024, stride, stride_acc);

            write_row(out, "sweep_stride",
                      base_cache_kb, base_line_size, 1, base_hit_lat, base_miss_pen,
                      base_policy, "stride_walk",
                      ws_kb, stride,
                      c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
        }
    }

    // -----------------------------
    // 7) Sweep miss penalty (timing sensitivity)
    // Miss rate stays same; AMAT changes.
    // -----------------------------
    {
        const uint64_t reuse_ws_kb = 24;
        for (size_t mp : {10, 25, 50, 75, 100, 150, 200, 300}) {
            Cache c(base_cache_kb * 1024, base_line_size, base_assoc, base_hit_lat, mp, base_policy);
            c.reset_stats();
            trace_reuse_working_set(c, reuse_ws_kb * 1024, step_word, reuse_passes);

            write_row(out, "sweep_miss_penalty",
                      base_cache_kb, base_line_size, base_assoc, base_hit_lat, mp,
                      base_policy, "reuse_working_set",
                      reuse_ws_kb, 0,
                      c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
        }
    }

    // -----------------------------
    // 8) Sweep hit latency (timing sensitivity)
    // Miss rate stays same; AMAT shifts by constant.
    // -----------------------------
    {
        const uint64_t reuse_ws_kb = 24;
        for (size_t hl : {1, 2, 3, 4, 5}) {
            Cache c(base_cache_kb * 1024, base_line_size, base_assoc, hl, base_miss_pen, base_policy);
            c.reset_stats();
            trace_reuse_working_set(c, reuse_ws_kb * 1024, step_word, reuse_passes);

            write_row(out, "sweep_hit_latency",
                      base_cache_kb, base_line_size, base_assoc, hl, base_miss_pen,
                      base_policy, "reuse_working_set",
                      reuse_ws_kb, 0,
                      c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
        }
    }

    // -----------------------------
    // 9) Policy sweep on locality-heavy workload (LRU should look good here)
    // -----------------------------
    {
        const uint64_t reuse_ws_kb = 24;
        for (ReplacementPolicy pol : {ReplacementPolicy::LRU, ReplacementPolicy::FIFO, ReplacementPolicy::RANDOM}) {
            Cache c(base_cache_kb * 1024, base_line_size, base_assoc, base_hit_lat, base_miss_pen, pol);
            c.reset_stats();
            trace_reuse_working_set(c, reuse_ws_kb * 1024, step_word, reuse_passes);

            write_row(out, "sweep_policy_locality",
                      base_cache_kb, base_line_size, base_assoc, base_hit_lat, base_miss_pen,
                      pol, "reuse_working_set",
                      reuse_ws_kb, 0,
                      c.get_miss_rate(), c.get_amat(), c.get_hits(), c.get_misses());
        }
    }

    std::cout << "Wrote results.csv\n";
    return 0;
}
