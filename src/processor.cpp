#include "processor.hpp"

Processor::Processor(CacheAbstract& cache) : cache_(cache), pc(0), running(true) {
    for (auto& r : x) r = 0;
}

void Processor::set_initial_state(uint32_t pc_init, const uint32_t regs[32]) {
    pc = pc_init;
    for (int i = 0; i < 32; ++i)
        x[i] = regs[i];
    x[0] = 0;
}

void Processor::run(uint32_t start_ra) {
    while (running) {
        if (pc == start_ra) break;

        uint32_t instr = cache_.read32(pc, AccessType::Instruction);
        Command cmd = parse(instr);

        validate_opcode(cmd);  // Проверка валидности opcode/funct

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

uint32_t Processor::read_mem(uint32_t addr, uint32_t size, bool is_signed) {
    uint32_t value = 0;

    switch (size) {
        case 1: value = cache_.read8(addr, AccessType::Data); break;
        case 2: 
            value  = cache_.read8(addr, AccessType::Data);
            value |= cache_.read8(addr + 1, AccessType::Data) << 8;
            break;
        case 4:
            value = cache_.read32(addr, AccessType::Data);
            break;
        default: throw std::runtime_error("Invalid memory size");
    }

    if (is_signed) {
        switch (size) {
            case 1: return int32_t(int8_t(value));
            case 2: return int32_t(int16_t(value));
            default: return value;
        }
    }
    return value;
}

void Processor::write_mem(uint32_t addr, uint32_t value, uint32_t size) {
    switch (size) {
        case 1: cache_.write8(addr, value & 0xFF); break;
        case 2:
            cache_.write8(addr, value & 0xFF);
            cache_.write8(addr + 1, (value >> 8) & 0xFF);
            break;
        case 4:
            cache_.write32(addr, value);
            break;
        default: throw std::runtime_error("Invalid memory size");
    }
}

uint32_t Processor::get_reg(int i) const {
    if (i < 0 || i >= 32)
        std::out_of_range("Invalid register index");
    return x[i];
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
    cmd.imm = 0;

    switch (cmd.opcode) {
        case 0x03: case 0x13: case 0x67: case 0x73:
            cmd.imm = int32_t(raw_instr) >> 20;
            break;
        case 0x23:
            cmd.imm = ((raw_instr >> 25) & 0x7F) << 5 | ((raw_instr >> 7) & 0x1F);
            if (cmd.imm & 0x800) cmd.imm |= 0xFFFFF000;
            break;
        case 0x63:
            cmd.imm = ((raw_instr >> 31) & 0x1) << 12
                    | ((raw_instr >> 25) & 0x3F) << 5
                    | ((raw_instr >> 8) & 0xF) << 1
                    | ((raw_instr >> 7) & 0x1) << 11;
            if (cmd.imm & 0x1000) cmd.imm |= 0xFFFFE000;
            break;
        case 0x17: case 0x37:
            cmd.imm = raw_instr & 0xFFFFF000;
            break;
        case 0x6F:
            cmd.imm = ((raw_instr >> 31) & 0x1) << 20
                    | ((raw_instr >> 21) & 0x3FF) << 1
                    | ((raw_instr >> 20) & 0x1) << 11
                    | ((raw_instr >> 12) & 0xFF) << 12;
            if (cmd.imm & 0x100000) cmd.imm |= 0xFFE00000;
            break;
    }

    return cmd;
}

void Processor::validate_opcode(const Command& c) {
    switch (c.opcode) {
        case 0x03: case 0x13: case 0x33:
        case 0x23: case 0x63: case 0x73:
        case 0x17: case 0x37: case 0x6F: case 0x67:
            return; // корректные opcode
        default:
            throw std::runtime_error("Invalid opcode: " + std::to_string(c.opcode));
    }
}

void Processor::write_reg(uint8_t rd, uint32_t value) {
    x[rd] = value;
    x[0] = 0;
}

std::function<void(Command&, Processor&)> Processor::get_function(const Command& cmd) {
    switch (cmd.opcode) {
        case 0x33: return [this](Command& c, Processor& p){ p.exec_r_type(c); };
        case 0x03: return [this](Command& c, Processor& p){ p.exec_load(c); };
        case 0x13: return [this](Command& c, Processor& p){ p.exec_imm_arith(c); };
        case 0x23: return [this](Command& c, Processor& p){ p.exec_store(c); };
        case 0x63: return [this](Command& c, Processor& p){ p.exec_branch(c); };
        case 0x73: return [this](Command& c, Processor& p){ p.exec_system(c); };
        case 0x17: return [this](Command& c, Processor& p){ p.exec_auipc(c); };
        case 0x37: return [this](Command& c, Processor& p){ p.exec_lui(c); };
        case 0x6F: return [this](Command& c, Processor& p){ p.exec_jal(c); };
        case 0x67: return [this](Command& c, Processor& p){ p.exec_jalr(c); };
        default: break;
    }
    throw std::runtime_error("End of get_function method in Processor");
}

void Processor::exec_r_type(Command& c) {
    switch (c.funct3) {
        case 0x0:
            if (c.funct7 == 0x00) write_reg(c.rd, x[c.rs1] + x[c.rs2]);      // ADD
            else if (c.funct7 == 0x20) write_reg(c.rd, x[c.rs1] - x[c.rs2]); // SUB
            else if (c.funct7 == 0x01) {                                     // MUL
                uint64_t res = uint64_t(uint32_t(x[c.rs1])) * uint64_t(uint32_t(x[c.rs2]));
                write_reg(c.rd, static_cast<uint32_t>(res & 0xFFFFFFFF));
            }
            break;
        case 0x1:
            if (c.funct7 == 0x00) write_reg(c.rd, x[c.rs1] << (x[c.rs2] & 0x1F)); // SLL
            else if (c.funct7 == 0x01) { // MULH
                int64_t res = int64_t(int32_t(x[c.rs1])) * int64_t(int32_t(x[c.rs2]));
                write_reg(c.rd, static_cast<uint32_t>((res >> 32) & 0xFFFFFFFF));
            }
            break;
        case 0x2: write_reg(c.rd, (int32_t)x[c.rs1] < (int32_t)x[c.rs2]); break; // SLT
        case 0x3: write_reg(c.rd, x[c.rs1] < x[c.rs2]); break;                     // SLTU
        case 0x4: write_reg(c.rd, x[c.rs1] ^ x[c.rs2]); break;                     // XOR
        case 0x5:
            if (c.funct7 == 0x00) write_reg(c.rd, x[c.rs1] >> (x[c.rs2] & 0x1F));   // SRL
            else if (c.funct7 == 0x20) write_reg(c.rd, int32_t(x[c.rs1]) >> (x[c.rs2] & 0x1F)); // SRA
            break;
        case 0x6: write_reg(c.rd, x[c.rs1] | x[c.rs2]); break; // OR
        case 0x7: write_reg(c.rd, x[c.rs1] & x[c.rs2]); break; // AND
    }
    pc += 4;
}

void Processor::exec_load(Command& c) {
    uint32_t addr = x[c.rs1] + c.imm;
    switch (c.funct3) {
        case 0x0: write_reg(c.rd, read_mem(addr, 1, true));  break; // LB
        case 0x1: write_reg(c.rd, read_mem(addr, 2, true));  break; // LH
        case 0x2: write_reg(c.rd, read_mem(addr, 4, false)); break; // LW
        case 0x4: write_reg(c.rd, read_mem(addr, 1, false)); break; // LBU
        case 0x5: write_reg(c.rd, read_mem(addr, 2, false)); break; // LHU
    }
    pc += 4;
}

void Processor::exec_imm_arith(Command& c) {
    switch (c.funct3) {
        case 0x0: write_reg(c.rd, x[c.rs1] + c.imm); break;           // ADDI
        case 0x2: write_reg(c.rd, (int32_t)x[c.rs1] < c.imm); break;   // SLTI
        case 0x3: write_reg(c.rd, x[c.rs1] < (uint32_t)c.imm); break; // SLTIU
        case 0x4: write_reg(c.rd, x[c.rs1] ^ c.imm); break;           // XORI
        case 0x6: write_reg(c.rd, x[c.rs1] | c.imm); break;           // ORI
        case 0x7: write_reg(c.rd, x[c.rs1] & c.imm); break;           // ANDI
        case 0x1: write_reg(c.rd, x[c.rs1] << (c.imm & 0x1F)); break; // SLLI
        case 0x5:
            if ((c.funct7 & 0x20) == 0) write_reg(c.rd, x[c.rs1] >> (c.imm & 0x1F)); // SRLI
            else write_reg(c.rd, int32_t(x[c.rs1]) >> (c.imm & 0x1F));                // SRAI
            break;
    }
    pc += 4;
}

void Processor::exec_store(Command& c) {
    uint32_t addr = x[c.rs1] + c.imm;
    switch (c.funct3) {
        case 0x0: write_mem(addr, x[c.rs2], 1); break; // SB
        case 0x1: write_mem(addr, x[c.rs2], 2); break; // SH
        case 0x2: write_mem(addr, x[c.rs2], 4); break; // SW
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
    }
    pc += take ? c.imm : 4;
}

void Processor::exec_system(Command& c) {
    if (c.funct3 == 0x0 && (c.funct12 == 0x0 || c.funct12 == 0x1)) {
        running = false; // ECALL/EBREAK
    }
    pc += 4;
}

void Processor::exec_lui(Command& c) { 
    write_reg(c.rd, c.imm); 
    pc += 4; 
}

void Processor::exec_auipc(Command& c) { 
    write_reg(c.rd, pc + c.imm); 
    pc += 4; 
}

void Processor::exec_jal(Command& c) { 
    write_reg(c.rd, pc + 4); 
    pc += c.imm; 
}

void Processor::exec_jalr(Command& c) { 
    uint32_t tmp = pc + 4; 
    pc = (x[c.rs1] + c.imm) & ~1; 
    write_reg(c.rd, tmp); 
}
