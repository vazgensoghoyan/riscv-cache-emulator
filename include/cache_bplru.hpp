#pragma once // cache_bplru.hpp

#include "cache_abstract.hpp"

class CacheBpLRU : public CacheAbstract {
public:
    explicit CacheBpLRU(RAM& ram);

private:
    uint32_t choose_victim(uint32_t set) override;
    void on_hit(uint32_t set, uint32_t way) override;
    void on_fill(uint32_t set, uint32_t way) override;

private:
    uint8_t used[CACHE_SET_COUNT][CACHE_WAY];
};
