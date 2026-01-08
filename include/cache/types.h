#pragma once
#include <cstddef>

struct MemoryTiming {
    // Cycles to first byte (queuing + DRAM + controller + interconnect, etc.)
    size_t fixed_latency_cycles = 0;

    // Sustained bandwidth model: how many bytes per cycle can be delivered
    // Example: 16 means a 128-bit bus per cycle (simplified).
    size_t bytes_per_cycle = 16;

    // Convert a transfer of N bytes into cycles (ceil division)
    size_t transfer_cycles(size_t bytes) const {
        if (bytes_per_cycle == 0) return 0;
        return (bytes + bytes_per_cycle - 1) / bytes_per_cycle;
    }

    // Total miss service time for fetching one cache line
    size_t miss_service_cycles(size_t line_size_bytes) const {
        return fixed_latency_cycles + transfer_cycles(line_size_bytes);
    }
};
