#include "processor.hpp"

void Processor::exec_r_type(Command& c) {
    switch (c.funct3) {
        case 0x0:
            if (c.funct7 == 0x01) { // MUL
                uint64_t res = uint64_t(uint32_t(x[c.rs1])) * uint64_t(uint32_t(x[c.rs2]));
                write_reg(c.rd, static_cast<uint32_t>(res & 0xFFFFFFFF));
            }
            break;
        case 0x1:
            if (c.funct7 == 0x01) { // MULH
                int64_t res = int64_t(int32_t(x[c.rs1])) * int64_t(int32_t(x[c.rs2]));
                write_reg(c.rd, static_cast<uint32_t>((res >> 32) & 0xFFFFFFFF));
            }
            break;
        case 0x4:
            if (c.funct7 == 0x01) { // DIV
                write_reg(c.rd, x[c.rs2] ? int32_t(x[c.rs1]) / int32_t(x[c.rs2]) : -1);
            }
            break;
        case 0x5:
            if (c.funct7 == 0x01) { // DIVU
                write_reg(c.rd, x[c.rs2] ? x[c.rs1] / x[c.rs2] : 0xFFFFFFFF);
            }
            break;
        case 0x6:
            if (c.funct7 == 0x01) { // REM
                write_reg(c.rd, x[c.rs2] ? int32_t(x[c.rs1]) % int32_t(x[c.rs2]) : x[c.rs1]);
            }
            break;
        case 0x7:
            if (c.funct7 == 0x01) { // REMU
                write_reg(c.rd, x[c.rs2] ? x[c.rs1] % x[c.rs2] : x[c.rs1]);
            }
            break;
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
