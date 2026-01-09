#pragma once
#include <cstdint>
#include <cstddef>
#include <iostream>
#include "cache/storage.h"
#include "cache/types.h"   // MemoryTiming (fixed latency + bytes_per_cycle)

class Memory : public IStorage {
public:
    // line_size_bytes here is the transfer granularity (we’ll use L2 line size typically)
    Memory(MemoryTiming timing, size_t line_size_bytes)
        : timing(timing), line_size(line_size_bytes) {}

    bool access(uint64_t address, uint64_t& latency) override {
        (void)address;
        accesses++;
        // Memory always "misses" in cache sense, but always services the request.
        latency = timing.miss_service_cycles(line_size);
        return false;
    }

    void reset_stats() override {
        accesses = 0;
    }

    uint64_t get_accesses() const { return accesses; }

    void print_stats() const {
        std::cout << "Memory accesses: " << accesses << "\n";
        std::cout << "Memory line_size: " << line_size << " bytes\n";
        std::cout << "Memory fixed_latency: " << timing.fixed_latency_cycles << " cycles\n";
        std::cout << "Memory bytes_per_cycle: " << timing.bytes_per_cycle << "\n";
    }

private:
    MemoryTiming timing{};
    size_t line_size = 64;
    uint64_t accesses = 0;
};
