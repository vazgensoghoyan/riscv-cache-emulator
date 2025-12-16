#include "processor.hpp"

Processor::Processor(CacheAbstract& cache) : cache_(cache), pc(0), running(true) {
    for (auto& r : x) r = 0;
}

void Processor::run(uint32_t start_ra) {
    while (running) {
        if (pc == start_ra) break;

        uint32_t instr = cache_.read32(pc, AccessType::Instruction);
        Command cmd = parse(instr);

        auto f = get_function(cmd);
        f(cmd, *this);
    }

    cache_.flush();
}

uint32_t Processor::read_mem32(uint32_t addr, AccessType type) {
    return cache_.read32(addr, type);
}

void Processor::write_mem32(uint32_t addr, uint32_t value) {
    cache_.write32(addr, value);
}

Command Processor::parse(uint32_t raw_instr) {
    Command cmd;
    cmd.raw = raw_instr;
    cmd.opcode = raw_instr & 0x7F;
    cmd.rd = (raw_instr >> 7) & 0x1F;
    cmd.funct3 = (raw_instr >> 12) & 0x07;
    cmd.rs1 = (raw_instr >> 15) & 0x1F;
    cmd.rs2 = (raw_instr >> 20) & 0x1F;
    cmd.funct7 = (raw_instr >> 25) & 0x7F;
    cmd.funct12 = (raw_instr >> 20) & 0xFFF;
    cmd.imm = 0; // Заполнить по типу инструкции
    return cmd;
}

std::function<void(Command&, Processor&)> Processor::get_function(const Command& cmd) {
    switch (cmd.opcode) {
        case 0x33: return [](Command& c, Processor& p){ p.exec_r_type(c); };
        case 0x03: return [](Command& c, Processor& p){ p.exec_load(c); };
        case 0x13: return [](Command& c, Processor& p){ p.exec_imm_arith(c); };
        case 0x23: return [](Command& c, Processor& p){ p.exec_store(c); };
        case 0x63: return [](Command& c, Processor& p){ p.exec_branch(c); };
        case 0x73: return [](Command& c, Processor& p){ p.exec_system(c); };
        case 0x17: return [](Command& c, Processor& p){ p.exec_auipc(c); };
        case 0x37: return [](Command& c, Processor& p){ p.exec_lui(c); };
        case 0x6F: return [](Command& c, Processor& p){ p.exec_jal(c); };
        case 0x67: return [](Command& c, Processor& p){ p.exec_jalr(c); };
        default:   return [](Command&, Processor&){ /* UNKNOWN */ };
    }
}

void Processor::exec_r_type(Command& c)       { /* ADD, SUB, MUL, ... */ }
void Processor::exec_load(Command& c)         { /* lb/lh/lw/lbu/lhu */ }
void Processor::exec_imm_arith(Command& c)    { /* addi, slti, sltiu, ... */ }
void Processor::exec_store(Command& c)        { /* sb/sh/sw */ }
void Processor::exec_branch(Command& c)       { /* beq, bne, blt, ... */ }
void Processor::exec_system(Command& c)       { /* ecall, ebreak, fence */ }
void Processor::exec_auipc(Command& c)        { /* AUIPC */ }
void Processor::exec_lui(Command& c)          { /* LUI */ }
void Processor::exec_jal(Command& c)          { /* JAL */ }
void Processor::exec_jalr(Command& c)         { /* JALR */ }
