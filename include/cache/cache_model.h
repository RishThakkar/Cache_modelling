#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <iostream>

#include "cache/replacement_policy.h"

class Cache {
public:
    Cache(size_t cache_size,
          size_t line_size,
          size_t associativity,
          size_t hit_latency,
          size_t miss_penalty,
          ReplacementPolicy policy = ReplacementPolicy::LRU);

    bool access(uint64_t address, uint64_t& latency);

    void print_stats() const;

    // --- Stats control ---
    void reset_stats();

    // --- Getters ---
    uint64_t get_hits() const;
    uint64_t get_misses() const;
    uint64_t get_accesses() const;
    double   get_miss_rate() const;
    double   get_amat() const;

private:
    size_t cache_size;
    size_t line_size;
    size_t associativity;
    size_t num_sets;

    size_t hit_latency;
    size_t miss_penalty;

    ReplacementPolicy policy;

    uint64_t access_counter = 0;

    struct CacheLine {
        bool valid = false;
        uint64_t tag = 0;

        // For replacement policies:
        uint64_t last_used = 0;    // LRU
        uint64_t inserted_at = 0;  // FIFO
    };

    struct CacheSet {
        std::vector<CacheLine> lines;
    };

    std::vector<CacheSet> sets;

    // Stats
    uint64_t hits = 0;
    uint64_t misses = 0;

    // RNG for RANDOM policy
    uint64_t rng_state = 0x9e3779b97f4a7c15ULL;

    uint64_t extract_tag(uint64_t addr) const;
    uint64_t extract_index(uint64_t addr) const;

    CacheLine* pick_victim(CacheSet& set);
    uint64_t next_rand();
};
