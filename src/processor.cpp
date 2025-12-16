#include "processor.hpp"

Processor::Processor(CacheAbstract& cache) : cache_(cache), pc(0), running(true) {
    for (auto& r : x) r = 0;
}

void Processor::set_initial_state(const uint32_t regs[32]) {
    pc = regs[0];
    for (int i = 0; i < 32; ++i)
        x[i] = regs[i];
    x[0] = 0;
}

void Processor::run() {
    uint32_t start_ra = x[1];
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
