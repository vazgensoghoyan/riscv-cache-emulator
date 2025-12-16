#include "cache_bplru.hpp"

CacheBpLRU::CacheBpLRU(RAM& ram) : CacheAbstract(ram) {
    std::memset(used, 0, sizeof(used));
}

uint32_t CacheBpLRU::choose_victim(uint32_t set) {
    for (uint32_t way = 0; way < CACHE_WAY; ++way) {
        if (used[set][way] == 0)
            return way;
    }

    for (uint32_t way = 0; way < CACHE_WAY; ++way) {
        used[set][way] = 0;
    }

    uint32_t victim = 0;
    used[set][victim] = 1;
    return victim;
}

void CacheBpLRU::on_hit(uint32_t set, uint32_t way) {
    used[set][way] = 1;

    bool all_used = true;
    for (uint32_t w = 0; w < CACHE_WAY; ++w) {
        if (used[set][w] == 0) {
            all_used = false;
            break;
        }
    }

    if (all_used) {
        for (uint32_t w = 0; w < CACHE_WAY; ++w)
            used[set][w] = 0;

        used[set][way] = 1;
    }
}

void CacheBpLRU::on_fill(uint32_t set, uint32_t way) {
    on_hit(set, way);
}
