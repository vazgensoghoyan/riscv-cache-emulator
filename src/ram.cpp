#include <ram.hpp>

// constructor

RAM::RAM(uint32_t size)
    : size_(size),
      data_(std::vector<uint8_t>(size, 0)),
      defined_(std::vector<bool>(size, false))
{
}

// private utils

void RAM::check_bounds(uint32_t addr, uint32_t bytes) const {
    if (addr + bytes > size_)
        throw std::out_of_range("Memory access out of bounds");
}

void RAM::check_defined(uint32_t addr, uint32_t bytes) const {
    for (uint32_t i = 0; i < bytes; ++i)
        if (!defined_[addr + i])
            throw std::runtime_error("Access to undefined memory");
}

// for reading from input.bin

void RAM::load_fragment(uint32_t addr, const std::vector<uint8_t>& data) {
    check_bounds(addr, static_cast<uint32_t>(data.size()));

    for (uint32_t i = 0; i < data.size(); ++i) {
        data_[addr + i] = data[i];
        defined_[addr + i] = true;
    }
}

// reading and writing

uint8_t RAM::read8(uint32_t addr) const {
    check_bounds(addr, 1);
    check_defined(addr, 1);
    return data_[addr];
}

void RAM::write8(uint32_t addr, uint8_t value) {
    check_bounds(addr, 1);

    data_[addr] = value;
    defined_[addr] = true;
}

// to write to output.bin

std::vector<uint8_t> RAM::dump(uint32_t addr, uint32_t size) const {
    check_bounds(addr, size);

    std::vector<uint8_t> result(size);
    for (uint32_t i = 0; i < size; ++i) {
        result[i] = data_[addr + i];
    }
    return result;
}
