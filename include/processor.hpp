#pragma once

#include <cstdint>

class Processor {
public:
    Processor() : pc(0), x({0}), running(true) { }

    uint32_t get_reg(uint8_t idx) const { return idx == 0 ? 0 : x[idx]; }
    void set_reg(uint8_t idx, uint32_t val) { if (idx != 0) x[idx] = val; }

private:
    uint32_t pc;
    uint32_t x[32];
    bool running;

};
