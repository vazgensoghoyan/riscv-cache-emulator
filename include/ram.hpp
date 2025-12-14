#ifndef RAM_HPP
#define RAM_HPP

#include <cstdint>
#include <vector>
#include <stdexcept>

class RAM {
public:
    explicit RAM(uint32_t size);

    void load_fragment(uint32_t addr, const std::vector<uint8_t>& data);

    uint8_t read8(uint32_t addr) const;
    void write8(uint32_t addr, uint8_t  value);

    std::vector<uint8_t> dump(uint32_t addr, uint32_t size) const;

    uint32_t size() const noexcept { return size_; }

private:
    void check_bounds(uint32_t addr, uint32_t bytes) const;
    void check_defined(uint32_t addr, uint32_t bytes) const;

    uint32_t size_;
    std::vector<uint8_t> data_;
    std::vector<bool> defined_;
};

#endif // RAM_HPP
