#pragma once // processor.hpp

#include <cstdint>
#include <functional>
#include <stdexcept>

#include "cache_abstract.hpp"

struct Command {
    uint32_t raw = 0;
    uint8_t opcode = 0;
    uint8_t rd = 0;
    uint8_t funct3 = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    uint8_t funct7 = 0;
    uint16_t funct12 = 0;
    int32_t imm = 0;
};

class Processor {
public:
    explicit Processor(CacheAbstract& cache);
    
    void set_initial_state(const uint32_t regs[32]);

    void run();

    uint32_t get_reg(int i) const;

private:
    Command parse(uint32_t raw_instr);
    std::function<void(Command&, Processor&)> get_function(const Command& cmd);

    void exec_r_type(Command& c);
    void exec_load(Command& c);
    void exec_imm_arith(Command& c);
    void exec_store(Command& c);
    void exec_branch(Command& c);
    void exec_system(Command& c);
    void exec_auipc(Command& c);
    void exec_lui(Command& c);
    void exec_jal(Command& c);
    void exec_jalr(Command& c);

    void validate_opcode(const Command& c);

    uint32_t read_mem(uint32_t addr, uint32_t size, bool is_signed);
    void write_mem(uint32_t addr, uint32_t value, uint32_t size);

    void write_reg(uint8_t rd, uint32_t value);

private:
    CacheAbstract& cache_;
    uint32_t pc;
    uint32_t x[32];
    bool running;
};
