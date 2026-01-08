#pragma once
#include <cstdint>

enum class ReplacementPolicy : uint8_t {
    LRU,
    FIFO,
    RANDOM
};

