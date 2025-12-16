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

void Processor::exec_r_type(Command& c) {
    switch (c.funct3) {
        case 0x0: // ADD/SUB
            if (c.funct7 == 0x00) x[c.rd] = x[c.rs1] + x[c.rs2]; // ADD
            else if (c.funct7 == 0x20) x[c.rd] = x[c.rs1] - x[c.rs2]; // SUB
            else if (c.funct7 == 0x01) x[c.rd] = (int64_t(x[c.rs1]) * int64_t(x[c.rs2])) & 0xFFFFFFFF; // MUL
            break;
        case 0x1: // SLL
            x[c.rd] = x[c.rs1] << (x[c.rs2] & 0x1F);
            break;
        case 0x2: // SLT
            x[c.rd] = int32_t(x[c.rs1]) < int32_t(x[c.rs2]);
            break;
        case 0x3: // SLTU
            x[c.rd] = x[c.rs1] < x[c.rs2];
            break;
        case 0x4: // XOR
            x[c.rd] = x[c.rs1] ^ x[c.rs2];
            break;
        case 0x5: // SRL/SRA
            if (c.funct7 == 0x00) x[c.rd] = x[c.rs1] >> (x[c.rs2] & 0x1F); // SRL
            else if (c.funct7 == 0x20) x[c.rd] = int32_t(x[c.rs1]) >> (x[c.rs2] & 0x1F); // SRA
            break;
        case 0x6: // OR
            x[c.rd] = x[c.rs1] | x[c.rs2];
            break;
        case 0x7: // AND
            x[c.rd] = x[c.rs1] & x[c.rs2];
            break;
        default:
            throw std::runtime_error("Unknown R-type instruction");
    }
    pc += 4;
}

void Processor::exec_load(Command& c) {
    uint32_t addr = x[c.rs1] + c.imm;
    switch (c.funct3) {
        case 0x0: x[c.rd] = int8_t(read_mem32(addr, AccessType::Data)); break; // LB
        case 0x1: x[c.rd] = int16_t(read_mem32(addr, AccessType::Data)); break; // LH
        case 0x2: x[c.rd] = read_mem32(addr, AccessType::Data); break;           // LW
        case 0x4: x[c.rd] = uint8_t(read_mem32(addr, AccessType::Data)); break;  // LBU
        case 0x5: x[c.rd] = uint16_t(read_mem32(addr, AccessType::Data)); break; // LHU
        default: throw std::runtime_error("Unknown load instruction");
    }
    pc += 4;
}

void Processor::exec_imm_arith(Command& c) {
    switch (c.funct3) {
        case 0x0: x[c.rd] = x[c.rs1] + c.imm; break;   // ADDI
        case 0x2: x[c.rd] = int32_t(x[c.rs1]) < c.imm; break; // SLTI
        case 0x3: x[c.rd] = uint32_t(x[c.rs1]) < uint32_t(c.imm); break; // SLTIU
        case 0x4: x[c.rd] = x[c.rs1] ^ c.imm; break;   // XORI
        case 0x6: x[c.rd] = x[c.rs1] | c.imm; break;   // ORI
        case 0x7: x[c.rd] = x[c.rs1] & c.imm; break;   // ANDI
        case 0x1: x[c.rd] = x[c.rs1] << (c.imm & 0x1F); break; // SLLI
        case 0x5:
            if ((c.funct7 & 0x20) == 0) x[c.rd] = x[c.rs1] >> (c.imm & 0x1F); // SRLI
            else x[c.rd] = int32_t(x[c.rs1]) >> (c.imm & 0x1F);                // SRAI
            break;
        default: throw std::runtime_error("Unknown I-type instruction");
    }
    pc += 4;
}

void Processor::exec_store(Command& c) {
    uint32_t addr = x[c.rs1] + c.imm;
    switch (c.funct3) {
        case 0x0: write_mem32(addr, x[c.rs2] & 0xFF); break; // SB
        case 0x1: write_mem32(addr, x[c.rs2] & 0xFFFF); break; // SH
        case 0x2: write_mem32(addr, x[c.rs2]); break;          // SW
        default: throw std::runtime_error("Unknown store instruction");
    }
    pc += 4;
}

void Processor::exec_branch(Command& c) {
    bool take = false;
    switch (c.funct3) {
        case 0x0: take = x[c.rs1] == x[c.rs2]; break; // BEQ
        case 0x1: take = x[c.rs1] != x[c.rs2]; break; // BNE
        case 0x4: take = int32_t(x[c.rs1]) < int32_t(x[c.rs2]); break; // BLT
        case 0x5: take = int32_t(x[c.rs1]) >= int32_t(x[c.rs2]); break; // BGE
        case 0x6: take = x[c.rs1] < x[c.rs2]; break; // BLTU
        case 0x7: take = x[c.rs1] >= x[c.rs2]; break; // BGEU
        default: throw std::runtime_error("Unknown branch instruction");
    }
    pc = take ? pc + c.imm : pc + 4;
}

void Processor::exec_system(Command& c) {
    // заглушка для ecall, ebreak, fence
    pc += 4;
}

void Processor::exec_auipc(Command& c) { 
    x[c.rd] = pc + c.imm; 
    pc += 4; 
}

void Processor::exec_lui(Command& c) { 
    x[c.rd] = c.imm; 
    pc += 4; 
}

void Processor::exec_jal(Command& c) { 
    x[c.rd] = pc + 4; 
    pc += c.imm; 
}

void Processor::exec_jalr(Command& c) { 
    uint32_t tmp = pc + 4; 
    pc = (x[c.rs1] + c.imm) & ~1; 
    x[c.rd] = tmp; 
}
