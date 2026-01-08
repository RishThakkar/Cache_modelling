#include "cache/cache_model.h"
#include <stdexcept>

Cache::Cache(size_t cache_size,
             size_t line_size,
             size_t associativity,
             size_t hit_latency,
             size_t miss_penalty,
             ReplacementPolicy policy)
    : cache_size(cache_size),
      line_size(line_size),
      associativity(associativity),
      hit_latency(hit_latency),
      miss_penalty(miss_penalty),
      policy(policy)
{
    if (line_size == 0 || associativity == 0) {
        throw std::invalid_argument("line_size and associativity must be > 0");
    }
    if (cache_size == 0 || cache_size % (line_size * associativity) != 0) {
        throw std::invalid_argument("cache_size must be a multiple of (line_size * associativity)");
    }

    num_sets = cache_size / (line_size * associativity);
    if (num_sets == 0) {
        throw std::invalid_argument("num_sets computed as 0 (check parameters)");
    }

    sets.resize(num_sets);
    for (auto& set : sets) {
        set.lines.resize(associativity);
    }

    // Seed RNG (simple): mix in some params so different configs vary.
    rng_state ^= (cache_size * 1315423911ULL) ^ (line_size * 2654435761ULL) ^ (associativity * 889523592379ULL);
}

Cache::Cache(size_t cache_size,
             size_t line_size,
             size_t associativity,
             size_t hit_latency,
             MemoryTiming mem_timing,
             ReplacementPolicy policy)
    : cache_size(cache_size),
      line_size(line_size),
      associativity(associativity),
      hit_latency(hit_latency),
      miss_penalty(0),          // not used when use_mem_timing=true
      policy(policy),
      mem_timing(mem_timing),
      use_mem_timing(true)
{
    if (line_size == 0 || associativity == 0) {
        throw std::invalid_argument("line_size and associativity must be > 0");
    }
    if (cache_size == 0 || cache_size % (line_size * associativity) != 0) {
        throw std::invalid_argument("cache_size must be a multiple of (line_size * associativity)");
    }

    num_sets = cache_size / (line_size * associativity);
    if (num_sets == 0) {
        throw std::invalid_argument("num_sets computed as 0 (check parameters)");
    }

    sets.resize(num_sets);
    for (auto& set : sets) {
        set.lines.resize(associativity);
    }

    rng_state ^= (cache_size * 1315423911ULL) ^ (line_size * 2654435761ULL) ^ (associativity * 889523592379ULL);
}


uint64_t Cache::extract_index(uint64_t addr) const {
    return (addr / line_size) % num_sets;
}

uint64_t Cache::extract_tag(uint64_t addr) const {
    return (addr / line_size) / num_sets;
}

// Simple xorshift64* RNG for RANDOM replacement
uint64_t Cache::next_rand() {
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 2685821657736338717ULL;
}

Cache::CacheLine* Cache::pick_victim(CacheSet& set) {
    // Prefer an invalid line first (empty slot)
    for (auto& line : set.lines) {
        if (!line.valid) return &line;
    }

    // Otherwise choose based on policy
    CacheLine* victim = &set.lines[0];

    switch (policy) {
        case ReplacementPolicy::LRU: {
            // Smallest last_used => least recently used
            for (auto& line : set.lines) {
                if (line.last_used < victim->last_used) victim = &line;
            }
            return victim;
        }

        case ReplacementPolicy::FIFO: {
            // Smallest inserted_at => oldest resident
            for (auto& line : set.lines) {
                if (line.inserted_at < victim->inserted_at) victim = &line;
            }
            return victim;
        }

        case ReplacementPolicy::RANDOM: {
            const uint64_t r = next_rand();
            const size_t way = static_cast<size_t>(r % set.lines.size());
            return &set.lines[way];
        }

        default:
            return victim;
    }
}

bool Cache::access(uint64_t address, uint64_t& latency) {
    access_counter++;

    const uint64_t index = extract_index(address);
    const uint64_t tag   = extract_tag(address);

    auto& set = sets[index];

    // Hit?
    for (auto& line : set.lines) {
        if (line.valid && line.tag == tag) {
            hits++;
            line.last_used = access_counter; // LRU touch
            latency = hit_latency;
            return true;
        }
    }

    // Miss
    misses++;
    latency = hit_latency + effective_miss_penalty_cycles();

    // Victim selection
    CacheLine* victim = pick_victim(set);

    // Fill/replace line
    victim->valid = true;
    victim->tag = tag;

    // Update replacement metadata
    victim->last_used = access_counter;     // for LRU
    victim->inserted_at = access_counter;   // for FIFO

    return false;
}

void Cache::print_stats() const {
    const uint64_t total = hits + misses;
    const double miss_rate = (total == 0) ? 0.0 : static_cast<double>(misses) / static_cast<double>(total);
    double amat = static_cast<double>(hit_latency) + miss_rate * static_cast<double>(effective_miss_penalty_cycles());

    std::cout << "Hits: " << hits << "\n";
    std::cout << "Misses: " << misses << "\n";
    std::cout << "Miss rate: " << miss_rate << "\n";
    std::cout << "AMAT: " << amat << " cycles\n";
}

void Cache::reset_stats() {
    hits = 0;
    misses = 0;
    access_counter = 0;

    // Flush cache lines
    for (auto& set : sets) {
        for (auto& line : set.lines) {
            line.valid = false;
            line.tag = 0;
            line.last_used = 0;
            line.inserted_at = 0;
        }
    }
}

uint64_t Cache::get_hits() const { return hits; }
uint64_t Cache::get_misses() const { return misses; }
uint64_t Cache::get_accesses() const { return hits + misses; }

double Cache::get_miss_rate() const {
    const uint64_t total = get_accesses();
    if (total == 0) return 0.0;
    return static_cast<double>(misses) / static_cast<double>(total);
}

double Cache::get_amat() const {
    return static_cast<double>(hit_latency) + get_miss_rate() * static_cast<double>(effective_miss_penalty_cycles());
}