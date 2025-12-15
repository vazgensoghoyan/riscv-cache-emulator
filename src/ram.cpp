#include "ram.hpp"

#include <stdexcept>

// constructor

RAM::RAM(uint32_t size)
    : size_(size),
      data_(new uint8_t[size])
{}

// reading

uint8_t RAM::read8(uint32_t address) const {
    if (address >= size_) {
        throw std::out_of_range("RAM read out of bounds");
    }
    return data_[address];
}

// writing

void RAM::write8(uint32_t address, uint8_t value) {
    if (address >= size_) {
        throw std::out_of_range("RAM write out of bounds");
    }
    data_[address] = value;
}
