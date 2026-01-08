#pragma once

#include <cstdint>   // uint64_t
#include <cstddef>   // size_t
#include <vector>
#include <iostream>


class Cache {
public:
    Cache(size_t cache_size,
          size_t line_size,
          size_t associativity,
          size_t hit_latency,
          size_t miss_penalty);

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

    uint64_t access_counter = 0;

    struct CacheLine {
        bool valid = false;
        uint64_t tag = 0;
        uint64_t last_used = 0; // for LRU
    };

    struct CacheSet {
        std::vector<CacheLine> lines;
    };

    std::vector<CacheSet> sets;

    // Stats
    uint64_t hits = 0;
    uint64_t misses = 0;

    uint64_t extract_tag(uint64_t addr) const;
    uint64_t extract_index(uint64_t addr) const;
};
