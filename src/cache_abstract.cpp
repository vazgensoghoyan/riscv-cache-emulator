#include "cache_abstract.hpp"

static inline uint32_t addr_offset(uint32_t a) {
    return a & ((1u << CACHE_OFFSET_LEN) - 1);
}

static inline uint32_t addr_index(uint32_t a) {
    return (a >> CACHE_OFFSET_LEN) & ((1u << CACHE_INDEX_LEN) - 1);
}

static inline uint32_t addr_tag(uint32_t a) {
    return a >> (CACHE_OFFSET_LEN + CACHE_INDEX_LEN);
}

static inline uint32_t line_base(uint32_t a) {
    return a & ~(CACHE_LINE_SIZE - 1);
}

// ctor

CacheAbstract::CacheAbstract(RAM& ram)
    : ram_(ram)
{
    std::memset(cache_, 0, sizeof(cache_));
}

// Public API

uint32_t CacheAbstract::read32(uint32_t addr, AccessType type) {
    if (type == AccessType::Instruction)
        stats_.instr_access++;
    else
        stats_.data_access++;

    Line& line = fetch_line(addr, type);
    uint32_t off = addr_offset(addr);

    uint32_t value;
    std::memcpy(&value, &line.data[off], sizeof(uint32_t));
    return value;
}

void CacheAbstract::write32(uint32_t addr, uint32_t value) {
    stats_.data_access++;

    Line& line = fetch_line(addr, AccessType::Data);
    uint32_t off = addr_offset(addr);

    std::memcpy(&line.data[off], &value, sizeof(uint32_t));
    line.dirty = true;
}

CacheAbstract::Line& CacheAbstract::fetch_line(uint32_t addr, AccessType type) {
    const uint32_t set = addr_index(addr);
    const uint32_t tag = addr_tag(addr);

    for (uint32_t way = 0; way < CACHE_WAY; ++way) {
        Line& line = cache_[set][way];
        if (line.valid && line.tag == tag) {
            if (type == AccessType::Instruction)
                stats_.instr_hit++;
            else
                stats_.data_hit++;

            on_hit(set, way);
            return line;
        }
    }

    for (uint32_t way = 0; way < CACHE_WAY; ++way) {
        if (!cache_[set][way].valid) {
            Line& line = cache_[set][way];

            uint32_t base = line_base(addr);
            for (uint32_t i = 0; i < CACHE_LINE_SIZE; ++i)
                line.data[i] = ram_.read8(base + i);

            line.valid = true;
            line.dirty = false;
            line.tag = tag;

            on_fill(set, way);
            return line;
        }
    }

    uint32_t way = choose_victim(set);
    Line& line = cache_[set][way];

    if (line.dirty) {
        uint32_t base =
            (line.tag << (CACHE_INDEX_LEN + CACHE_OFFSET_LEN)) |
            (set << CACHE_OFFSET_LEN);

        for (uint32_t i = 0; i < CACHE_LINE_SIZE; ++i)
            ram_.write8(base + i, line.data[i]);
    }

    uint32_t base = line_base(addr);
    for (uint32_t i = 0; i < CACHE_LINE_SIZE; ++i)
        line.data[i] = ram_.read8(base + i);

    line.valid = true;
    line.dirty = false;
    line.tag = tag;

    on_fill(set, way);
    return line;
}

void CacheAbstract::flush() {
    for (uint32_t set = 0; set < CACHE_SET_COUNT; ++set) {
        for (uint32_t way = 0; way < CACHE_WAY; ++way) {
            Line& line = cache_[set][way];
            if (!line.valid || !line.dirty) continue;

            uint32_t base =
                (line.tag << (CACHE_INDEX_LEN + CACHE_OFFSET_LEN)) |
                (set << CACHE_OFFSET_LEN);

            for (uint32_t i = 0; i < CACHE_LINE_SIZE; ++i)
                ram_.write8(base + i, line.data[i]);

            line.dirty = false;
        }
    }
}
