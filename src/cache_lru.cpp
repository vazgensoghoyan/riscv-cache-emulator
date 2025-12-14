#include "cache_lru.hpp"

CacheLRU::CacheLRU(RAM& ram) : CacheAbstract(ram) {
    reset_policy();
}

void CacheLRU::reset_policy() {
    for (uint32_t set = 0; set < CACHE_SET_COUNT; ++set)
        for (uint32_t way = 0; way < CACHE_WAY; ++way)
            lru_age[set][way] = way; // 0 = недавно, 3 = LRU
}

uint32_t CacheLRU::choose_victim(uint32_t set) {
    // выбрать путь с наибольшим возрастом
    uint32_t victim = 0;
    uint8_t max_age = lru_age[set][0];
    for (uint32_t way = 1; way < CACHE_WAY; ++way) {
        if (lru_age[set][way] > max_age) {
            max_age = lru_age[set][way];
            victim = way;
        }
    }
    return victim;
}

void CacheLRU::on_hit(uint32_t set, uint32_t way) {
    uint8_t old_age = lru_age[set][way];
    // линии с возрастом < old_age → не меняем, остальные +=1
    for (uint32_t w = 0; w < CACHE_WAY; ++w) {
        if (w == way)
            lru_age[set][w] = 0; // сброс для линии hit
        else if (lru_age[set][w] < old_age)
            ; // младше → остаются
        else
            lru_age[set][w]++;
    }
}

void CacheLRU::on_fill(uint32_t set, uint32_t way) {
    // fill аналогично hit
    for (uint32_t w = 0; w < CACHE_WAY; ++w) {
        if (w == way)
            lru_age[set][w] = 0;
        else
            lru_age[set][w]++;
    }
}
