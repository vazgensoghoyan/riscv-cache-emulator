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

// reading from ram

uint8_t RAM::read8(uint32_t addr) const {
    check_bounds(addr, 1);
    check_defined(addr, 1);
    return data_[addr];
}

uint16_t RAM::read16(uint32_t addr) const {
    check_bounds(addr, 2);
    check_defined(addr, 2);

    return static_cast<uint16_t>(
          data_[addr]
        | (data_[addr + 1] << 8)
    );
}

uint32_t RAM::read32(uint32_t addr) const {
    check_bounds(addr, 4);
    check_defined(addr, 4);

    return static_cast<uint32_t>(
          data_[addr]
        | (data_[addr + 1] << 8)
        | (data_[addr + 2] << 16)
        | (data_[addr + 3] << 24)
    );
}

// writing to ram

void RAM::write8(uint32_t addr, uint8_t value) {
    check_bounds(addr, 1);

    data_[addr] = value;
    defined_[addr] = true;
}

void RAM::write16(uint32_t addr, uint16_t value) {
    check_bounds(addr, 2);

    data_[addr]     = static_cast<uint8_t>(value & 0xFF);
    data_[addr + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);

    defined_[addr]     = true;
    defined_[addr + 1] = true;
}

void RAM::write32(uint32_t addr, uint32_t value) {
    check_bounds(addr, 4);

    data_[addr] = static_cast<uint8_t>(value & 0xFF);
    data_[addr + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data_[addr + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    data_[addr + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);

    defined_[addr] = true;
    defined_[addr + 1] = true;
    defined_[addr + 2] = true;
    defined_[addr + 3] = true;
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
