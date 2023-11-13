#include <iostream>
#include <stdint.h>

using namespace std;

uint8_t MEMORY[0xFFF] ={0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
                        0x20, 0x60, 0x20, 0x20, 0x70, // 1
                        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
                        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
                        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
                        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
                        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
                        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
                        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
                        0xE0, 0x90, 0xF0, 0x90, 0x90, // A
                        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
                        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
                        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
                        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
                        0xF0, 0x80, 0xF0, 0x80, 0x80, // F
                        
};                                                                          // Emulated memory space for 4095 (0xFFF) bytes of RAM memory
  
uint8_t V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, VA, VB, VC, VD, VE;         // 8-bit registers - used as variables
uint8_t VF;                                                                 // 8-bit flag register - also a variable, but used as a flag by some instructions

uint8_t X;                                                                  // 4-bit constant register identifier (it is an 8-bit data type, but will mask for only the first 4 bits for 0x0 to 0xF)
uint8_t Y;                                                                  // 4-bit constant register identifier (it is an 8-bit data type, but will mask for only the first 4 bits for 0x0 to 0xF)
uint8_t NN;                                                                 // 8-bit constant, for setting V-register values or doing conditionals

uint16_t I;                                                                 // 16-bit index register for memory addresses - Only rightmost 12 bits used since only 4096 memory available
uint16_t NNN;                                                               // addr, memory address
uint16_t PC;                                                                // Program Counter - stores currently executing address

uint16_t STACK[16] = {};                                                    // Call Stack for subroutines/functions, Holds 16-bit values
uint8_t SP;                                                                 // Stack Pointer - points to the top level of the stack

uint8_t DELAY;                                                              // 8-bit delay timer, count down at 60Hz until it reaches 0. Value can be set and read
uint8_t SOUND;                                                              // 8-bit soudn timer, count down at 60Hz until it reaches 0. Beep is played when value is non-zero

int main() {

    return 0;
}