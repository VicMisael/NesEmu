#ifndef _CPU_H_
#define _CPU_H_
#include "MMU.h"

class CPU
{
private:
    struct
    {
        halfword PC;
        uint8_t SP;
        status_register_t sr;
        uint8_t AC;
        uint8_t X;
        uint8_t Y;

    } registers;

    MMU *mmu;
    void execute(uint8_t &inst, uint32_t &cycles);

public:
    CPU(MMU *mmu)
    {
        this->mmu = mmu;
    }
    ~CPU(){};
    void run();
};

#endif