#pragma once
#include <cstdint>

struct IStorage {
    virtual ~IStorage() = default;

    // Returns true if this level hits, false if it misses
    // latency is the total latency contributed by this level (and below, if miss)
    virtual bool access(uint64_t address, uint64_t& latency) = 0;

    virtual void reset_stats() = 0;
};
