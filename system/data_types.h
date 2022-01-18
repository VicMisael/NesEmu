#ifndef _DATATYPES_
#define _DATATYPES_
#include <cstdint>

union nes_byte
{
    uint8_t _unsigned;
    int8_t _signed;
};
union halfword
{
    uint16_t value;
    struct
    {
        uint8_t ll; // LSB;
        uint8_t hh; // MSB;
    };
};

enum branching_modes
{
    carry_clear,
    carry_set,
    zero_set,
    negative_set,
    zero_clear,
    negative_clear,
    overflow_clear,
    overflow_set
};

union status_register_t
{
    struct
    {
        // Carry flag
        bool C : 1; // LSB
        // Zero Flag
        bool Z : 1;
        // Interrupt Flag
        bool I : 1;
        // Decimal Flag
        bool D : 1;
        // Break Flag
        bool B : 1;
        bool ignored : 1;
        // Overflow
        bool V : 1;
        // Negative
        bool N : 1; // MSB
    };
    uint8_t value;
};
union instruction_t
{
    uint8_t _instruction;
    struct
    {
        uint8_t _c : 2; // defines the instruction
        uint8_t _b : 3; // defines the addressing mode
        uint8_t _a : 3; // defines the instruction
    };
};
#endif