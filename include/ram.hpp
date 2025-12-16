#pragma once

#include <cstdint>

class RAM {
public:
    explicit RAM(uint32_t size);
    ~RAM();

    uint8_t read8(uint32_t address) const;
    void write8(uint32_t address, uint8_t value);

    uint32_t size() const noexcept { return size_; }

private:
    uint32_t size_;
    uint8_t* data_;
};
