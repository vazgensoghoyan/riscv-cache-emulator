#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>

#include "processor.hpp"
#include "cache_lru.hpp"
#include "cache_bplru.hpp"
#include "ram.hpp"
#include "config.hpp"

struct InputData {
    uint32_t registers[32]{};
    std::map<uint32_t, std::vector<uint8_t>> memory;
};

InputData read_input_file(const std::string& filename) {
    InputData input;

    std::ifstream in(filename, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open input file");

    in.read(reinterpret_cast<char*>(input.registers), sizeof(input.registers));
    if (!in) throw std::runtime_error("Cannot read registers");

    while (true) {
        uint32_t addr, size;
        in.read(reinterpret_cast<char*>(&addr), sizeof(addr));
        if (!in) break; // EOF
        in.read(reinterpret_cast<char*>(&size), sizeof(size));
        if (!in) throw std::runtime_error("Corrupted memory fragment header");

        std::vector<uint8_t> data(size);
        in.read(reinterpret_cast<char*>(data.data()), size);
        if (!in) throw std::runtime_error("Corrupted memory fragment data");

        input.memory[addr] = std::move(data);
    }

    return input;
}

void print_stats(const char* name, const CacheStats& s) {
    uint64_t total_access = s.instr_access + s.data_access;
    uint64_t total_hit    = s.instr_hit + s.data_hit;

    if (total_access == 0) {
        std::printf(
            "| %-11s |       nan%% |              nan%% |             nan%% | %12d | %12d | %12d | %12d |\n",
            name, 0, 0, 0, 0
        );
        return;
    }

    double hit_rate = 100.0 * total_hit / total_access;
    double instr_hit_rate = s.instr_access ? 100.0 * s.instr_hit / s.instr_access : 0.0;
    double data_hit_rate  = s.data_access  ? 100.0 * s.data_hit  / s.data_access  : 0.0;

    std::printf(
        "| %-11s | %3.4f%% |       %3.4f%% |      %3.4f%% | %12llu | %12llu | %12llu | %12llu |\n",
        name,
        hit_rate,
        instr_hit_rate,
        data_hit_rate,
        (unsigned long long)s.instr_access,
        (unsigned long long)s.instr_hit,
        (unsigned long long)s.data_access,
        (unsigned long long)s.data_hit
    );
}

void load_memory(RAM& ram, const std::map<uint32_t, std::vector<uint8_t>>& memory) {
    for (const auto& [addr, data] : memory) {
        for (size_t i = 0; i < data.size(); ++i) {
            if (addr + i >= ram.size()) throw std::runtime_error("Memory address out of range");
            ram.write8(addr + i, data[i]);
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 3 || std::string(argv[1]) != "-i") {
            throw std::runtime_error("Usage: ./emulator -i <input_file>");
        }

        std::string input_file = argv[2];
        InputData input = read_input_file(input_file);

        // ------------------ Создание RAM ------------------
        RAM ram(MEMORY_SIZE);
        load_memory(ram, input.memory);

        CacheLRU cache_lru(ram);
        Processor cpu_lru(cache_lru);

        cpu_lru.run(input.registers[1]);

        cpu_lru.run(input.registers[0]); // pc из input

        print_stats("LRU", cache_lru.stats());

        CacheBpLRU cache_bplru(ram);
        Processor cpu_bplru(cache_bplru);

        cpu_bplru.run(input.registers[0]);

        print_stats("bpLRU", cache_bplru.stats());

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
