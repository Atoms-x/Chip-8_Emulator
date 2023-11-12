#include <iostream>
#include <stdint.h>

using namespace std;

int main() {
    uint8_t V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, VA, VB, VC, VD, VE;     // 8-bit registers - used as variables
    uint8_t VF;                                                              // 8-bit register - also a variable, but used as a flag by some instructions

    uint16_t I;                                                             // 16-bit register for memory addresses - Only rightmost 12 bits used since only 4096 memory available
    uint16_t PC;                                                            // Program Counter - stores currently executing address

    V0 = 0x48;
    I = 0x1000;
    cout << "There might be register now. V0 = " << V0 << endl;
    cout << "Memory address at 0x" << I << endl;

    return 0;
}