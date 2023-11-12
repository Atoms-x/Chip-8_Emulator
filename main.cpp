#include <iostream>
#include <stdint.h>

using namespace std;

int main() {
    uint8_t V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, VA, VB, VC, VD, VE;         // 8-bit registers - used as variables
    uint8_t VF;                                                                 // 8-bit register - also a variable, but used as a flag by some instructions

    uint8_t X;                                                                  // 4-bit constant register identifier (it is an 8-bit data type, but will mask for only the first 4 bits for 0x0 to 0xF)
    uint8_t Y;                                                                  // 4-bit constant register identifier (it is an 8-bit data type, but will mask for only the first 4 bits for 0x0 to 0xF)
    uint8_t NN;                                                                 // 8-bit constant, for setting V-register values or doing conditionals

    uint16_t I;                                                                 // 16-bit register for memory addresses - Only rightmost 12 bits used since only 4096 memory available
    uint16_t NNN;                                                               // addr, memory address
    uint16_t PC;                                                                // Program Counter - stores currently executing address

    uint16_t STACK[16] = {};                                                    // Call Stack, Holds 16-bit values
    uint8_t SP;                                                                 // Stack Pointer - points to the top level of the stack

    uint8_t DELAY;                                                              // 8-bit delay timer, count down at 60Hz until it reaches 0. Value can be set and read
    uint8_t SOUND;                                                              // 8-bit soudn timer, count down at 60Hz until it reaches 0. Beep is played when value is non-zero

    SP = 3;
    NNN = 0xF12;
    V0 = 0x48;
    I = 0xFFF;
    cout << "There might be register now. V0 = " << V0 << endl;
    cout << "Memory address at 0x" << I << endl;
    cout << "Stack Pointer = " << SP << endl;
    cout << "Top of Stack is: " << STACK[SP] << endl;

    return 0;
}