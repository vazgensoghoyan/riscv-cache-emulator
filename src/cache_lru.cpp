#include "cache_lru.hpp"

CacheLRU::CacheLRU(RAM& ram) : CacheAbstract(ram) {
    for (uint32_t set = 0; set < CACHE_SET_COUNT; ++set) {
        for (uint32_t way = 0; way < CACHE_WAY; ++way) {
            last_used[set][way] = way;
        }
    }
}

uint32_t CacheLRU::choose_victim(uint32_t set) {
    for (uint32_t way = 0; way < CACHE_WAY; ++way) {
        if (last_used[set][way] == CACHE_WAY - 1)
            return way;
    }
    return 0;
}

void CacheLRU::on_hit(uint32_t set, uint32_t way) {
    uint8_t old = last_used[set][way];

    for (uint32_t w = 0; w < CACHE_WAY; ++w) {
        if (last_used[set][w] < old)
            last_used[set][w]++;
    }

    last_used[set][way] = 0;
}

void CacheLRU::on_fill(uint32_t set, uint32_t way) {
    for (uint32_t w = 0; w < CACHE_WAY; ++w) {
        last_used[set][w]++;
    }

    last_used[set][way] = 0;
}
