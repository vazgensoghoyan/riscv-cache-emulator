#pragma once // cache_lru.hpp

#include "cache_abstract.hpp"

class CacheLRU : public CacheAbstract {
public:
    explicit CacheLRU(RAM& ram);

private:
    uint32_t choose_victim(uint32_t set) override;
    void on_hit(uint32_t set, uint32_t way) override;
    void on_fill(uint32_t set, uint32_t way) override;

private:
    uint8_t last_used[CACHE_SET_COUNT][CACHE_WAY];
};
