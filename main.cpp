#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <cmath>

#include "processor.hpp"
#include "cache_lru.hpp"
#include "cache_bplru.hpp"
#include "ram.hpp"
#include "config.hpp"

struct InputData {
    std::vector<uint32_t> registers;
    std::map<uint32_t, std::vector<uint8_t>> memory;
};

InputData read_input_file(const std::string& filename) {
    InputData input;

    std::ifstream in(filename, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open input file");

    input.registers.resize(32);
    if (!in.read(reinterpret_cast<char*>(input.registers.data()), 32 * sizeof(uint32_t))) {
        throw std::runtime_error("Cannot read registers from input file");
    }

    while (true) {
        uint32_t addr = 0;
        uint32_t size = 0;

        if (!in.read(reinterpret_cast<char*>(&addr), sizeof(addr))) {
            break;
        }

        if (!in.read(reinterpret_cast<char*>(&size), sizeof(size))) {
            throw std::runtime_error("Corrupted memory fragment header (size missing)");
        }

        if (size > 0) {
            std::vector<uint8_t> data(size);
            if (!in.read(reinterpret_cast<char*>(data.data()), size)) {
                throw std::runtime_error("Corrupted memory fragment data");
            }
            input.memory[addr] = std::move(data);
        }
    }

    return input;
}

void print_stats(const char* name, const CacheStats& s) {
    uint64_t total_access = s.instr_access + s.data_access;
    uint64_t total_hit = s.instr_hit + s.data_hit;

    double hit_rate = total_access ? 100.0 * total_hit / total_access : std::nan("");

    double instr_hit_rate = s.instr_access ? 100.0 * s.instr_hit / s.instr_access : std::nan("");
    double data_hit_rate = s.data_access ? 100.0 * s.data_hit / s.data_access  : std::nan("");

    std::printf(
        "| %-11s | %3.4f%% |       %3.4f%% |      %3.4f%% | %12d | %12d | %12d | %12d |\n",
        name,
        hit_rate,
        instr_hit_rate,
        data_hit_rate,
        (int)s.instr_access,
        (int)s.instr_hit,
        (int)s.data_access,
        (int)s.data_hit
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

void write_output_file(const std::string& filename,
                       const Processor& cpu,
                       RAM& ram,
                       uint32_t start_addr,
                       uint32_t size) {
    if (start_addr >= ram.size())
        throw std::runtime_error("Start address out of RAM bounds");
    if (size == 0 || start_addr + size > ram.size())
        throw std::runtime_error("Memory size out of RAM bounds");

    std::ofstream out(filename, std::ios::binary);
    if (!out)
        throw std::runtime_error("Cannot open output file");

    for (int i = 0; i < 32; ++i) {
        uint32_t val = cpu.get_reg(i);
        out.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    out.write(reinterpret_cast<const char*>(&start_addr), sizeof(start_addr));
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));

    for (uint32_t i = 0; i < size; ++i) {
        uint8_t byte = ram.read8(start_addr + i);
        out.write(reinterpret_cast<const char*>(&byte), sizeof(byte));
    }

    out.close();
}

int main(int argc, char* argv[]) {
    try {
        std::string input_file;
        std::string output_file;
        uint32_t out_addr = 0, out_size = 0;
        bool has_output = false;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-i") {
                if (i + 1 >= argc) throw std::runtime_error("Missing input file after -i");
                input_file = argv[++i];
            } else if (arg == "-o") {
                if (i + 3 >= argc) throw std::runtime_error("Missing arguments for -o");
                output_file = argv[++i];
                out_addr = std::stoul(argv[++i], nullptr, 0); // поддержка 0x
                out_size = std::stoul(argv[++i], nullptr, 0);
                has_output = true;
            } else {
                throw std::runtime_error("Unknown argument: " + arg);
            }
        }

        InputData input = read_input_file(input_file);

        RAM ram_lru(MEMORY_SIZE);
        load_memory(ram_lru, input.memory);

        CacheLRU cache_lru(ram_lru);
        Processor cpu_lru(cache_lru, input.registers);
        cpu_lru.run();

        RAM ram_bplru(MEMORY_SIZE);
        load_memory(ram_bplru, input.memory);

        CacheBpLRU cache_bplru(ram_bplru);
        Processor cpu_bplru(cache_bplru, input.registers);
        cpu_bplru.run();

        std::printf("| replacement | hit_rate | instr_hit_rate | data_hit_rate | instr_access |  instr_hit   | data_access  |   data_hit   |\n");
        std::printf("| :---------- | :------: | -------------: | ------------: | -----------: | -----------: | -----------: | -----------: |\n");

        print_stats("LRU", cache_lru.stats());
        print_stats("bpLRU", cache_bplru.stats());

        if (has_output)
            write_output_file(output_file, cpu_lru, ram_lru, out_addr, out_size);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
