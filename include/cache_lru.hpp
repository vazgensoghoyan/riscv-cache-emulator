#ifndef CACHE_LRU_HPP
#define CACHE_LRU_HPP

#include "cache_abstract.hpp"

class CacheLRU : public CacheAbstract {
public:
    explicit CacheLRU(RAM& ram);
    
private:
    void reset_policy() override;
    uint32_t choose_victim(uint32_t set) override;
    void on_hit(uint32_t set, uint32_t way) override;
    void on_fill(uint32_t set, uint32_t way) override;

private:
    // для каждого сета храним массив "возраста" для 4 путей
    uint8_t lru_age[CACHE_SET_COUNT][CACHE_WAY];
};

#endif // CACHER_LRU_HPP
