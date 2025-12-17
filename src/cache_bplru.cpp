#include "cache_bplru.hpp"

CacheBpLRU::CacheBpLRU(RAM& ram) : CacheAbstract(ram) {
    std::memset(used, 0, sizeof(used));
}

uint32_t CacheBpLRU::choose_victim(uint32_t set) {
    for (uint32_t way = 0; way < CACHE_WAY; ++way) {
        if (!used[set][way])
            return way;
    }

    for (uint32_t way = 0; way < CACHE_WAY; ++way) {
        used[set][way] = false;
    }

    uint32_t victim = 0;
    used[set][victim] = true;
    return victim;
}

void CacheBpLRU::on_hit(uint32_t set, uint32_t way) {
    used[set][way] = true;

    bool all_used = true;
    for (uint32_t w = 0; w < CACHE_WAY; ++w) {
        if (!used[set][w]) {
            all_used = false;
            break;
        }
    }

    if (all_used) {
        for (uint32_t w = 0; w < CACHE_WAY; ++w)
            used[set][w] = false;

        used[set][way] = true;
    }
}

void CacheBpLRU::on_fill(uint32_t set, uint32_t way) {
    on_hit(set, way);
}
