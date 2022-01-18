#include "CPU.h"
#include "data_types.h"
#include <functional>
#include <unordered_map>
// Rotation
#define RIGHT true
#define LEFT false

/*
    x_indexed_indirect = 0,
    zeropage = 1,
    immediate = 2,
    absolute = 3,
    indirect_y_indexed = 4,
    zeropage_x_indexed = 5,
    absolute_y_indexed = 6,
    absolute_x_indexed = 7
*/

void ADC(uint8_t &ac, uint8_t val, status_register_t &status)
{
    bool carry = (ac + val + status.C) > 0xFF;
    uint8_t sum = (ac + val + status.C) & 0xff;
    status.V = (~(ac ^ val) & (ac ^ sum) & 0x80) > 0;
    ac = sum;
    status.N = ac >> 7;
    status.Z = ac == 0;
    status.C = carry;
};

void SBC(uint8_t &ac, uint8_t val, status_register_t &status)
{
    ADC(ac, ~val, status);
}
void AND(uint8_t &ac, uint8_t val, status_register_t &status)
{
    ac &= val;
    status.N = ac & 0x80;
    status.Z = ac == 0;
}
void EOR(uint8_t &ac, uint8_t val, status_register_t &status)
{
    ac ^= val;
    status.N = ac & 0x80;
    status.Z = ac == 0;
}
void ORA(uint8_t &ac, uint8_t val, status_register_t &status)
{
    ac |= val;
    status.N = ac & 0x80;
    status.Z = ac == 0;
}

void rotate(uint8_t &var, status_register_t &status, bool direction)
{
    bool oldC = status.C;
    if (direction)
    {
        status.C = (var & 0x80) > 0;
        var <<= 1;
        var = var || oldC;
    }
    else
    {
        status.C = (var & 1) > 0;
        var >>= 1;
        var = var || (oldC << 7);
    }

    status.Z = (var == 0);
    status.N = (var >> 7);
}

void ASL(uint8_t &var, status_register_t &status)
{
    status.C = (var & 0x80) != 0;
    var <<= 1;
    status.N = var >> 7;
    status.Z = var == 0;
}

void LSR(uint8_t &var, status_register_t &status)
{
    status.C = var & 1;
    var >> 1;
    status.N = 0;
    status.Z = var == 0;
}

void inline transfer_load(uint8_t &dest, uint8_t &from, status_register_t &status)
{
    dest = from;
    status.N = (dest >> 7) & 0x1;
    status.Z = dest == 0;
}

void LDA(uint8_t &dest, uint8_t from, status_register_t &status)
{
    uint8_t value = from;
    transfer_load(dest, value, status);
}
// Special case of transfer which the flags are not set
void TXS(uint8_t &SP, uint8_t &var, status_register_t &status)
{
    SP = var;
}

void BIT(uint8_t &ac, uint8_t &mem, status_register_t &status)
{
    uint8_t bits = (ac & mem);
}

void CMP(uint8_t &reg, uint8_t &mem, status_register_t &status)
{
    uint16_t result = reg - mem;
    status.N = (result >> 7);
    status.Z = result == 0;
    status.C = result > 255;
}

void decrement(uint8_t &var, status_register_t &status)
{
    var--;
    status.N = ((var >> 7) & 0x1);
    status.Z = var == 0;
}
void increment(uint8_t &var, status_register_t &status)
{
    var++;
    status.N = ((var >> 7) & 0x1);
    status.Z = var == 0;
}

/**
 * @brief Returns whether it shouold branch or not,this should cover all of the branching functions
 *
 * @param opcode
 * @param status
 * @return true
 * @return false
 */
bool branch(instruction_t &opcode, status_register_t &status)
{
    using std::function;
    using std::unordered_map;
    static unordered_map<int, function<bool(status_register_t)>> branchingInstructions;
    if (branchingInstructions.empty())
    {
        // BPL
        branchingInstructions[0] = [&](status_register_t status) -> bool
        { return !status.N; };
        // BMI
        branchingInstructions[1] = [&](status_register_t status) -> bool
        { return status.N; };
        // BVC
        branchingInstructions[2] = [&](status_register_t status) -> bool
        { return !status.V; };
        // BVS
        branchingInstructions[3] = [&](status_register_t status) -> bool
        { return status.V; };
        // BCC
        branchingInstructions[4] = [&](status_register_t status) -> bool
        { return !status.C; };
        // BCS
        branchingInstructions[5] = [&](status_register_t status) -> bool
        { return status.C; };
        // BNE
        branchingInstructions[6] = [&](status_register_t status) -> bool
        { return !status.Z; };
        // BEQ
        branchingInstructions[7] = [&](status_register_t status) -> bool
        { return status.Z; };
    }
    return branchingInstructions[opcode._b](status);
}

void CPU::execute(uint8_t &inst, uint32_t &cycles)
{
    const instruction_t *instruction = reinterpret_cast<instruction_t *>(&inst);
    const int operation = instruction->_a;
    const int addressingMode = instruction->_b;
    const int grouping = instruction->_c;
    uint16_t &PC = (registers.PC.value);
    switch (grouping)
    {
    case 0:
        break;
    case 1:
        switch (operation)
        {
        case 0:
            // ORA
            switch (addressingMode)
            {
            case 0:
            {
                // X indexed indirect
                halfword indirect;
                uint8_t value = mmu->read(PC++);
                indirect.hh = value + registers.X + 1;
                indirect.ll = value + registers.X;
                ORA(registers.AC, mmu->read(indirect.value), registers.sr);
                cycles += 6;
            }
            break;
            case 1:
            {
                // Zeropage
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                ORA(registers.AC, mmu->read(zeropage.value), registers.sr);
                cycles += 3;
            }
            break;
            case 2:
            { // Immediate
                ORA(registers.AC, (mmu->read(PC++)), registers.sr);
                cycles += 2;
            }

            break;
            case 3:
            { // Absolute
                halfword absolute;
                absolute.ll = mmu->read(PC++);
                absolute.hh = mmu->read(PC++);
                ORA(registers.AC, mmu->read(absolute.value), registers.sr);
                cycles += 4;
            }
            break;
            case 4:
            {
                // Indirect Y indexed
                halfword indirectY;
                uint8_t value = mmu->read(PC++);
                indirectY.hh = value;
                indirectY.ll = value;
                indirectY.value += registers.Y;
                ORA(registers.AC, mmu->read(indirectY.value), registers.sr);
                cycles += 5 + mmu->checkBoundaryCross();
            }

            break;
            case 5:
            {
                // zeropage x Indexed
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                zeropage.ll += registers.X;
                ORA(registers.AC, mmu->read(zeropage.value), registers.sr);
                cycles += 4;
            }
            break;
            case 6:
            {
                // Absolute y indexed
                halfword absoluteyIndexedY;
                absoluteyIndexedY.ll = mmu->read(PC++);
                absoluteyIndexedY.hh = mmu->read(PC++);
                absoluteyIndexedY.value += registers.Y;
                ORA(registers.AC, mmu->read(absoluteyIndexedY.value), registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
            }

            break;
            case 7:
            {
                halfword absoluteyIndexedX;
                absoluteyIndexedX.ll = mmu->read(PC++);
                absoluteyIndexedX.hh = mmu->read(PC++);
                absoluteyIndexedX.value += registers.Y;
                ORA(registers.AC, mmu->read(absoluteyIndexedX.value), registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
                //  Absolute X indexed
            }
            break;
            }
            break;
        case 1:
            // AND
            switch (addressingMode)
            {
            case 0:
            {
                // X indexed indirect
                halfword indirect;
                uint8_t value = mmu->read(PC++);
                indirect.hh = value + registers.X + 1;
                indirect.ll = value + registers.X;
                AND(registers.AC, mmu->read(indirect.value), registers.sr);
                cycles += 6;
            }
            break;
            case 1:
            {
                // Zeropage
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                AND(registers.AC, mmu->read(zeropage.value), registers.sr);
                cycles += 3;
            }
            break;
            case 2:
            { // Immediate
                AND(registers.AC, (mmu->read(PC++)), registers.sr);
                cycles += 2;
            }

            break;
            case 3:
            { // Absolute
                halfword absolute;
                absolute.ll = mmu->read(PC++);
                absolute.hh = mmu->read(PC++);
                AND(registers.AC, mmu->read(absolute.value), registers.sr);
                cycles += 4;
            }
            break;
            case 4:
            {
                // Indirect Y indexed
                halfword indirectY;
                uint8_t value = mmu->read(PC++);
                indirectY.hh = value;
                indirectY.ll = value;
                indirectY.value += registers.Y;
                AND(registers.AC, mmu->read(indirectY.value), registers.sr);
                cycles += 5 + mmu->checkBoundaryCross();
            }

            break;
            case 5:
            {
                // zeropage x Indexed
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                zeropage.ll += registers.X;
                AND(registers.AC, mmu->read(zeropage.value), registers.sr);
                cycles += 4;
            }
            break;
            case 6:
            {
                // Absolute y indexed
                halfword absoluteyIndexedY;
                absoluteyIndexedY.ll = mmu->read(PC++);
                absoluteyIndexedY.hh = mmu->read(PC++);
                absoluteyIndexedY.value += registers.Y;
                AND(registers.AC, mmu->read(absoluteyIndexedY.value), registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
            }

            break;
            case 7:
            {
                halfword absoluteyIndexedX;
                absoluteyIndexedX.ll = mmu->read(PC++);
                absoluteyIndexedX.hh = mmu->read(PC++);
                absoluteyIndexedX.value += registers.Y;
                AND(registers.AC, mmu->read(absoluteyIndexedX.value), registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
                //  Absolute X indexed
            }
            break;
            }
            break;
        case 2:
            // EOR
            switch (addressingMode)
            {
            case 0:
            {
                // X indexed indirect
                halfword indirect;
                uint8_t value = mmu->read(PC++);
                indirect.hh = value + registers.X + 1;
                indirect.ll = value + registers.X;
                EOR(registers.AC, mmu->read(indirect.value), registers.sr);
                cycles += 6;
            }
            break;
            case 1:
            {
                // Zeropage
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                EOR(registers.AC, mmu->read(zeropage.value), registers.sr);
                cycles += 3;
            }
            break;
            case 2:
            { // Immediate
                EOR(registers.AC, (mmu->read(PC++)), registers.sr);
                cycles += 2;
            }

            break;
            case 3:
            { // Absolute
                halfword absolute;
                absolute.ll = mmu->read(PC++);
                absolute.hh = mmu->read(PC++);
                EOR(registers.AC, mmu->read(absolute.value), registers.sr);
                cycles += 4;
            }
            break;
            case 4:
            {
                // Indirect Y indexed
                halfword indirectY;
                uint8_t value = mmu->read(PC++);
                indirectY.hh = value;
                indirectY.ll = value;
                indirectY.value += registers.Y;
                EOR(registers.AC, mmu->read(indirectY.value), registers.sr);
                cycles += 5 + mmu->checkBoundaryCross();
            }

            break;
            case 5:
            {
                // zeropage x Indexed
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                zeropage.ll += registers.X;
                EOR(registers.AC, mmu->read(zeropage.value), registers.sr);
                cycles += 4;
            }
            break;
            case 6:
            {
                // Absolute y indexed
                halfword absoluteyIndexedY;
                absoluteyIndexedY.ll = mmu->read(PC++);
                absoluteyIndexedY.hh = mmu->read(PC++);
                absoluteyIndexedY.value += registers.Y;
                EOR(registers.AC, mmu->read(absoluteyIndexedY.value), registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
            }

            break;
            case 7:
            {
                halfword absoluteyIndexedX;
                absoluteyIndexedX.ll = mmu->read(PC++);
                absoluteyIndexedX.hh = mmu->read(PC++);
                absoluteyIndexedX.value += registers.Y;
                EOR(registers.AC, mmu->read(absoluteyIndexedX.value), registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
                //  Absolute X indexed
            }
            break;
            }
            break;
        case 3:
            // ADC
            switch (addressingMode)
            {
            case 0:
            {
                // X indexed indirect
                halfword indirect;
                uint8_t value = mmu->read(PC++);
                indirect.hh = value + registers.X + 1;
                indirect.ll = value + registers.X;
                ADC(registers.AC, mmu->read(indirect.value), registers.sr);
                cycles += 6;
            }
            break;
            case 1:
            {
                // Zeropage
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                ADC(registers.AC, mmu->read(zeropage.value), registers.sr);
                cycles += 3;
            }
            break;
            case 2:
            { // Immediate
                ADC(registers.AC, (mmu->read(PC++)), registers.sr);
                cycles += 2;
            }

            break;
            case 3:
            { // Absolute
                halfword absolute;
                absolute.ll = mmu->read(PC++);
                absolute.hh = mmu->read(PC++);
                ADC(registers.AC, mmu->read(absolute.value), registers.sr);
                cycles += 4;
            }
            break;
            case 4:
            {
                // Indirect Y indexed
                halfword indirectY;
                uint8_t value = mmu->read(PC++);
                indirectY.hh = value;
                indirectY.ll = value;
                indirectY.value += registers.Y;
                ADC(registers.AC, mmu->read(indirectY.value), registers.sr);
                cycles += 5 + mmu->checkBoundaryCross();
            }

            break;
            case 5:
            {
                // zeropage x Indexed
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                zeropage.ll += registers.X;
                ADC(registers.AC, mmu->read(zeropage.value), registers.sr);
                cycles += 4;
            }
            break;
            case 6:
            {
                // Absolute y indexed
                halfword absoluteyIndexedY;
                absoluteyIndexedY.ll = mmu->read(PC++);
                absoluteyIndexedY.hh = mmu->read(PC++);
                absoluteyIndexedY.value += registers.Y;
                ADC(registers.AC, mmu->read(absoluteyIndexedY.value), registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
            }

            break;
            case 7:
            {
                halfword absoluteyIndexedX;
                absoluteyIndexedX.ll = mmu->read(PC++);
                absoluteyIndexedX.hh = mmu->read(PC++);
                absoluteyIndexedX.value += registers.Y;
                ADC(registers.AC, mmu->read(absoluteyIndexedX.value), registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
                //  Absolute X indexed
            }
            break;
            }
            break;
        case 4:
            // STA
            switch (addressingMode)
            {
            case 0:
            {
                // X indexed indirect
            }
            break;
            case 1:
            {
                // Zeropage
            }
            break;
            case 2:
            { // Immediate
              // Illegal
            }

            break;
            case 3:
            { // Absolute
            }
            break;
            case 4:
            {
                // Indirect Y indexed
            }

            break;
            case 5:
            {
                // zeropage x Indexed
            }
            break;
            case 6:
            {
                // Absolute y indexed
            }

            break;
            case 7:
            {
                //  Absolute X indexed
                
            }
            break;
            }
            break;
        case 5:
            // LDA
            switch (addressingMode)
            {
            case 0:
            {
                // X indexed indirect
                halfword indirect;
                uint8_t value = mmu->read(PC++);
                indirect.hh = value + registers.X + 1;
                indirect.ll = value + registers.X;
                LDA(registers.AC, mmu->read(indirect.value), registers.sr);
                cycles += 6;
            }
            break;
            case 1:
            {
                // Zeropage
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                LDA(registers.AC, mmu->read(zeropage.value), registers.sr);
                cycles += 3;
            }
            break;
            case 2:
            { // Immediate
                LDA(registers.AC, (mmu->read(PC++)), registers.sr);
                cycles += 2;
            }

            break;
            case 3:
            { // Absolute
                halfword absolute;
                absolute.ll = mmu->read(PC++);
                absolute.hh = mmu->read(PC++);
                LDA(registers.AC, mmu->read(absolute.value), registers.sr);
                cycles += 4;
            }
            break;
            case 4:
            {
                // Indirect Y indexed
                halfword indirectY;
                uint8_t value = mmu->read(PC++);
                indirectY.hh = value;
                indirectY.ll = value;
                indirectY.value += registers.Y;
                LDA(registers.AC, mmu->read(indirectY.value), registers.sr);
                cycles += 5 + mmu->checkBoundaryCross();
            }

            break;
            case 5:
            {
                // zeropage x Indexed
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                zeropage.ll += registers.X;
                LDA(registers.AC, mmu->read(zeropage.value), registers.sr);
                cycles += 4;
            }
            break;
            case 6:
            {
                // Absolute y indexed
                halfword absoluteyIndexedY;
                absoluteyIndexedY.ll = mmu->read(PC++);
                absoluteyIndexedY.hh = mmu->read(PC++);
                absoluteyIndexedY.value += registers.Y;
                LDA(registers.AC, mmu->read(absoluteyIndexedY.value), registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
            }

            break;
            case 7:
            {
                halfword absoluteyIndexedX;
                absoluteyIndexedX.ll = mmu->read(PC++);
                absoluteyIndexedX.hh = mmu->read(PC++);
                absoluteyIndexedX.value += registers.Y;
                LDA(registers.AC, mmu->read(absoluteyIndexedX.value), registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
                //  Absolute X indexed
            }
            break;
            }
            break;
        case 6:
            // CMP
            switch (addressingMode)
            {
            case 0:
            {
                // X indexed indirect
                halfword indirect;
                uint8_t value = mmu->read(PC++);
                indirect.hh = value + registers.X + 1;
                indirect.ll = value + registers.X;
                uint8_t value = mmu->read(indirect.value);
                CMP(registers.AC, value, registers.sr);
                cycles += 6;
            }
            break;
            case 1:
            {
                // Zeropage
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                uint8_t value = mmu->read(zeropage.value);
                CMP(registers.AC, value, registers.sr);
                cycles += 3;
            }
            break;
            case 2:
            { // Immediate
                uint8_t value = (mmu->read(PC++));
                CMP(registers.AC, value, registers.sr);
                cycles += 2;
            }

            break;
            case 3:
            { // Absolute
                halfword absolute;
                absolute.ll = mmu->read(PC++);
                absolute.hh = mmu->read(PC++);
                uint8_t value = mmu->read(absolute.value);
                CMP(registers.AC, value, registers.sr);
                cycles += 4;
            }
            break;
            case 4:
            {
                // Indirect Y indexed
                halfword indirectY;
                uint8_t value = mmu->read(PC++);
                indirectY.hh = value;
                indirectY.ll = value;
                indirectY.value += registers.Y;
                uint8_t value = mmu->read(indirectY.value);
                CMP(registers.AC, value, registers.sr);
                cycles += 5 + mmu->checkBoundaryCross();
            }

            break;
            case 5:
            {
                // zeropage x Indexed
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                zeropage.ll += registers.X;
                uint8_t value = mmu->read(zeropage.value);
                CMP(registers.AC, value, registers.sr);
                cycles += 4;
            }
            break;
            case 6:
            {
                // Absolute y indexed
                halfword absoluteyIndexedY;
                absoluteyIndexedY.ll = mmu->read(PC++);
                absoluteyIndexedY.hh = mmu->read(PC++);
                absoluteyIndexedY.value += registers.Y;
                uint8_t value = mmu->read(absoluteyIndexedY.value);
                CMP(registers.AC, value, registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
            }

            break;
            case 7:
            {
                halfword absoluteyIndexedX;
                absoluteyIndexedX.ll = mmu->read(PC++);
                absoluteyIndexedX.hh = mmu->read(PC++);
                absoluteyIndexedX.value += registers.Y;
                uint8_t value = mmu->read(absoluteyIndexedX.value);
                CMP(registers.AC, value, registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
                //  Absolute X indexed
            }
            break;
            }
        case 7:
            // SBC
            switch (addressingMode)
            {
            case 0:
            {
                // X indexed indirect
                halfword indirect;
                uint8_t value = mmu->read(PC++);
                indirect.hh = value + registers.X + 1;
                indirect.ll = value + registers.X;
                SBC(registers.AC, mmu->read(indirect.value), registers.sr);
                cycles += 6;
            }
            break;
            case 1:
            {
                // Zeropage
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                SBC(registers.AC, mmu->read(zeropage.value), registers.sr);
                cycles += 3;
            }
            break;
            case 2:
            { // Immediate
                SBC(registers.AC, (mmu->read(PC++)), registers.sr);
                cycles += 2;
            }

            break;
            case 3:
            { // Absolute
                halfword absolute;
                absolute.ll = mmu->read(PC++);
                absolute.hh = mmu->read(PC++);
                SBC(registers.AC, mmu->read(absolute.value), registers.sr);
                cycles += 4;
            }
            break;
            case 4:
            {
                // Indirect Y indexed
                halfword indirectY;
                uint8_t value = mmu->read(PC++);
                indirectY.hh = value;
                indirectY.ll = value;
                indirectY.value += registers.Y;
                SBC(registers.AC, mmu->read(indirectY.value), registers.sr);
                cycles += 5 + mmu->checkBoundaryCross();
            }

            break;
            case 5:
            {
                // zeropage x Indexed
                halfword zeropage;
                zeropage.hh = 00;
                zeropage.ll = mmu->read(PC++);
                zeropage.ll += registers.X;
                SBC(registers.AC, mmu->read(zeropage.value), registers.sr);
                cycles += 4;
            }
            break;
            case 6:
            {
                // Absolute y indexed
                halfword absoluteyIndexedY;
                absoluteyIndexedY.ll = mmu->read(PC++);
                absoluteyIndexedY.hh = mmu->read(PC++);
                absoluteyIndexedY.value += registers.Y;
                SBC(registers.AC, mmu->read(absoluteyIndexedY.value), registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
            }

            break;
            case 7:
            {
                halfword absoluteyIndexedX;
                absoluteyIndexedX.ll = mmu->read(PC++);
                absoluteyIndexedX.hh = mmu->read(PC++);
                absoluteyIndexedX.value += registers.Y;
                SBC(registers.AC, mmu->read(absoluteyIndexedX.value), registers.sr);
                cycles += (4 + mmu->checkBoundaryCross());
                //  Absolute X indexed
            }
            break;
            }
            break;
        }
        break;
    case 2:
        break;
    case 3:
        break;
    }
}
void CPU::run() {}