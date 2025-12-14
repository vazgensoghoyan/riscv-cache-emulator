#ifndef CACHE_ABSTRACT_HPP
#define CACHE_ABSTRACT_HPP

#include <cstdint>
#include <cstring>

#include "ram.hpp"
#include "config.hpp"

struct CacheStats {
    uint64_t instr_access = 0;
    uint64_t instr_hit = 0;
    uint64_t data_access = 0;
    uint64_t data_hit = 0;
};

enum class AccessType {
    Instruction,
    Data
};

class CacheAbstract {
public:
    explicit CacheAbstract(RAM& ram);
    virtual ~CacheAbstract() = default;

    virtual void reset_policy() = 0;

    uint32_t read32(uint32_t addr, AccessType access_type);
    void write32(uint32_t addr, uint32_t value);

    void flush(); // all changed data write back to ram
    
    CacheStats stats() const { return stats_; }

protected:
    struct Line {
        uint8_t data[CACHE_LINE_SIZE];
        bool valid = false;
        bool dirty = false;
        uint32_t tag = 0;
    };

protected:
    virtual uint32_t choose_victim(uint32_t set) = 0;
    virtual void on_hit(uint32_t set, uint32_t way) = 0;
    virtual void on_fill(uint32_t set, uint32_t way) = 0;

    Line& fetch_line(uint32_t addr, AccessType access_type);

protected:
    RAM& ram_;
    CacheStats stats_;

    Line cache_[CACHE_SET_COUNT][CACHE_WAY];
};

#endif // CACHE_ABSTRACT_HPP
