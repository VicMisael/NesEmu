#ifndef _MMU_
#define _MMU_
#include "data_types.h"
#include "Utils.h"
class MMU
{
    uint8_t Memory[2048];

public:
    // Reads a value from memory.
    const uint8_t read(uint16_t address)
    {
        using utils::isBetween;
        if (isBetween(0x0000, 0x1FFF, address))
        {
            return Memory[address % 0x800];
        }
        return 00;
    }
    void write(uint16_t address, nes_byte value)
    {
        using utils::isBetween;
        if (isBetween(0x0000, 0x1FFF, address))
        {
            Memory[address % 0x800] = value._unsigned;
        }
    }
    /*
     * checks if the last reading caused a boundary cross
     */
    bool checkBoundaryCross()
    {
        return false;
    }
};
#endif