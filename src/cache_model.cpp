#include "cache/cache_model.h"
#include <stdexcept>

Cache::Cache(size_t cache_size,
             size_t line_size,
             size_t associativity,
             size_t hit_latency,
             size_t miss_penalty)
    : cache_size(cache_size),
      line_size(line_size),
      associativity(associativity),
      hit_latency(hit_latency),
      miss_penalty(miss_penalty)
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
}

uint64_t Cache::extract_index(uint64_t addr) const {
    return (addr / line_size) % num_sets;
}

uint64_t Cache::extract_tag(uint64_t addr) const {
    return (addr / line_size) / num_sets;
}

bool Cache::access(uint64_t address, uint64_t& latency) {
    access_counter++;

    uint64_t index = extract_index(address);
    uint64_t tag   = extract_tag(address);

    auto& set = sets[index];

    // Hit?
    for (auto& line : set.lines) {
        if (line.valid && line.tag == tag) {
            hits++;
            line.last_used = access_counter;
            latency = hit_latency;
            return true;
        }
    }

    // Miss
    misses++;
    latency = hit_latency + miss_penalty;

    // Victim (LRU, prefer invalid)
    CacheLine* victim = &set.lines[0];
    for (auto& line : set.lines) {
        if (!line.valid) {
            victim = &line;
            break;
        }
        if (line.last_used < victim->last_used) {
            victim = &line;
        }
    }

    victim->valid = true;
    victim->tag = tag;
    victim->last_used = access_counter;

    return false;
}

void Cache::print_stats() const {
    uint64_t total = hits + misses;
    double miss_rate = (total == 0) ? 0.0 : static_cast<double>(misses) / static_cast<double>(total);

    double amat = static_cast<double>(hit_latency) + miss_rate * static_cast<double>(miss_penalty);

    std::cout << "Hits: " << hits << "\n";
    std::cout << "Misses: " << misses << "\n";
    std::cout << "Miss rate: " << miss_rate << "\n";
    std::cout << "AMAT: " << amat << " cycles\n";
}

// --- Stats control ---
void Cache::reset_stats() {
    hits = 0;
    misses = 0;
    access_counter = 0;

    // OPTIONAL: If you want to "flush" the cache too, uncomment below.
    // This makes each experiment start with an empty cache.
    for (auto& set : sets) {
        for (auto& line : set.lines) {
            line.valid = false;
            line.tag = 0;
            line.last_used = 0;
        }
    }
}

// --- Getters ---
uint64_t Cache::get_hits() const {
    return hits;
}

uint64_t Cache::get_misses() const {
    return misses;
}

uint64_t Cache::get_accesses() const {
    return hits + misses;
}

double Cache::get_miss_rate() const {
    const uint64_t total = get_accesses();
    if (total == 0) return 0.0;
    return static_cast<double>(misses) / static_cast<double>(total);
}

double Cache::get_amat() const {
    // AMAT = hit_latency + miss_rate * miss_penalty
    return static_cast<double>(hit_latency) + get_miss_rate() * static_cast<double>(miss_penalty);
}