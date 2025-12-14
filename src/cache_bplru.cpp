#include "cache_bplru.hpp"

CacheBpLRU::CacheBpLRU(RAM& ram) : CacheAbstract(ram) {
    reset_policy();
}

// сброс бит → все линии «левые» на старте
void CacheBpLRU::reset_policy() {
    std::memset(plru_bits, 0, sizeof(plru_bits));
}

// выбор линии-жертвы по битам
uint32_t CacheBpLRU::choose_victim(uint32_t set) {
    uint8_t bits = plru_bits[set];
    uint32_t way = 0;

    // b0
    if (bits & 0b100) { // правая ветка
        way |= 0b10;
    }
    // b1 или b2
    if (bits & 0b010) { // b1, левая правая
        way |= 0b01; // w1
    } else if (bits & 0b001) { // b2, правая ветка
        way |= 0b11; // w3
    }

    return way & 0x3; // 0..3
}

// обновление бит при hit
void CacheBpLRU::on_hit(uint32_t set, uint32_t way) {
    uint8_t& bits = plru_bits[set];

    switch (way) {
        case 0: bits &= ~0b100; bits &= ~0b010; break; // w0
        case 1: bits &= ~0b100; bits |=  0b010; break; // w1
        case 2: bits |=  0b100; bits &= ~0b001; break; // w2
        case 3: bits |=  0b100; bits |=  0b001; break; // w3
    }
}

// при fill — ведем себя так же, как hit
void CacheBpLRU::on_fill(uint32_t set, uint32_t way) {
    on_hit(set, way);
}
