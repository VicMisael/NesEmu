#include "system/data_types.h"
#include "system/CPU.h"
#include <iostream>
#include <unordered_map>
#include <functional>
#include <cmath>

int main(int argc, char *argv[])
{

    CPU cpu(new MMU());
    cpu.run();
}