#ifndef CACHE_BPLRU_HPP
#define CACHE_BPLRU_HPP

#include "cache_abstract.hpp"

class CacheBpLRU : public CacheAbstract {
public:
    explicit CacheBpLRU(RAM& ram);

private:
    uint32_t choose_victim(uint32_t set) override;
    void on_hit(uint32_t set, uint32_t way) override;
    void on_fill(uint32_t set, uint32_t way) override;

private:
    uint8_t plru_bits[CACHE_SET_COUNT]; // 3 бита на сет для 4-way
};

#endif // CACHE_BPLRU_HPP
