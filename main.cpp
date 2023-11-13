#include <iostream>
#include <fstream>
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
uint16_t PC = 0x200;                                                        // Program Counter - stores currently executing address

uint16_t STACK[16] = {};                                                    // Call Stack for subroutines/functions, Holds 16-bit values
uint8_t SP = 0;                                                             // Stack Pointer - points to the top level of the stack

uint8_t DELAY;                                                              // 8-bit delay timer, count down at 60Hz until it reaches 0. Value can be set and read
uint8_t SOUND;                                                              // 8-bit soudn timer, count down at 60Hz until it reaches 0. Beep is played when value is non-zero


//***************************************************************************
// void fileReadToMemory
//  Arguments:
//      string filename: The name of the file given by user
//  Returns:
//      Nothing
//  Purpose:
//      Loads the file bytes directly into memory starting at 0x200
//***************************************************************************
void fileReadToMemory (string filename){
    ifstream ch8Rom;
    ch8Rom.open(filename, ios::binary | ios::in);

    if(!ch8Rom){
        cout << "FILE NOT FOUND" << endl;
        exit(1);
    }
    else{
        ch8Rom.seekg(0, ch8Rom.end);                                        // seekg() sets/moves a position within the open file, utilizing this to get the file size in bytes with tellg()
        int romLength = ch8Rom.tellg();
        ch8Rom.seekg(0, ch8Rom.beg);
        
        uint8_t* rBuffer = new uint8_t[romLength];                          // create a buffer to read bytes from the file and eventually into memory

        while (ch8Rom.read(reinterpret_cast<char*>(rBuffer), romLength)){   // reinterpret_cast allows the use of a uint8_t buffer since it can alter a pointer type to integer type, 
            int j = 0x200;                                                  // despite the elements being read as chars. This keeps the data from being signed
            for (int i=0; i < romLength; i++){                           
                MEMORY[j] = rBuffer[i];                                     // Game memory starts at 0x200 (512) because the previous memory locations are normally saved for the Chip-8 interpreter and font
                j++;
            }
        }
        delete[] rBuffer;
    }
    if (ch8Rom.eof()){                                                      // Check for issue reading the file
        ch8Rom.close();
    }
    else if (ch8Rom.fail()){
        cout << "Error reading the file!" << endl;
        exit(1);
    }
}

//***************************************************************************
// void fetch
//  Arguments:
//      s
//  Returns:
//      N
//  Purpose:
//      L
//***************************************************************************
uint16_t fetch(){
    if (!(PC >= 0xFFF)){

        uint16_t byteOne = MEMORY[PC];                                      // Putting each byte of the 2-byte Opcodes into 16bit integers so they can be masked as the full Opcode
        uint16_t byteTwo = MEMORY[PC+1];

        byteOne = byteOne << 8;                                             // Bit shifting the first byte by 8 to put it into the left most 8-bits (first two nibbles of four for Opcode)

        uint16_t opCode = byteOne + byteTwo;                                // Adding the two together to make the full 2-byte Opcode

        PC += 2;                                                            // Shift the program counter by 2 so that it moves to the beginning of the next 2-byte Opcode

        return opCode;
    }
    else{
        cout << "Memory out of bounds!" << endl;
        exit(1);
    }
}

//***************************************************************************
// void decode_Execute
//  Arguments:
//      s
//  Returns:
//      N
//  Purpose:
//      L
//***************************************************************************
void decode_Execute(uint16_t opCode){
    uint16_t nibOne;                                                        // Creating four nibbles that are 16-bit so they can be switch/cased by each digit, working down what Opcode to execute
    uint16_t nibTwo;
    uint16_t nibThree;
    uint16_t nibFour;

    nibOne = opCode & 0xF000;                                               // Masking the nibbles with an AND operator to leave the appropriate nibble/hex digit in place for checking
    nibTwo = opCode & 0x0F00;
    nibThree = opCode & 0x00F0;
    nibFour = opCode & 0x000F;

    switch (nibOne)
    {
    case 0x0000:
        switch (nibTwo)
        {
        case 0x0000:
            switch (nibThree)
            {
            case 0x00E0:
                switch (nibFour)
                {
                case 0x0000:
                    cout << "NEED TO WRITE: Clears the Screen";             // Opcode 00E0 - Clears the screen
                    break;
                case 0x000E:
                    PC = STACK[SP];                                         // Opcode 00EE - Returns from a subroutine/sets PC to NNN/addr at top of stack, then decrements the stack pointer/stack
                    SP -= 1;
                    break;
                default:
                    cout << "Code Error!";
                    exit(1);
                    break;
                }
                break;
            default:
                cout << "Code Error!";
                exit(1);
                break;
            }
            break;
        default:
            cout << "This is the 0NNN Opcode";                              // Opcode 0NNN - Call machine code routine at address NNN
            break;
        }
        break;
    case 0x1000:
        PC = (nibTwo + nibThree + nibFour);                                 // Opcode 1NNN - Set PC to NNN/Jump to address NNN
        break;
    case 0x2000:
        SP += 1;                                                            // Opcode 2NNN - Calls subroutine at NNN/put PC onto the stack and set PC to NNN
        STACK[SP] = PC;
        PC = (nibTwo + nibThree + nibFour);
        break;
    case 0x3000:
        switch (nibTwo)                                                     // Opcode 3XNN - Skip the next instruction if VX == NN/increment PC by 2 if equal
        {
        case 0x0000:
            if (V0 == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0100:
            if (V1 == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0200:
            if (V2 == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0300:
            if (V3 == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0400:
            if (V4 == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0500:
            if (V5 == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0600:
            if (V6 == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0700:
            if (V7 == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0800:
            if (V8 == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0900:
            if (V9 == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0A00:
            if (VA == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0B00:
            if (VB == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0C00:
            if (VC == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0D00:
            if (VD == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0E00:
            if (VE == nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0F00:
            if (VF == nibThree + nibFour){
                PC += 2;
            }
            break;
        default:
            break;
        }
    case 0x4000:
        switch (nibTwo)                                                     // Opcode 4XNN - Skip the next instruction if VX != NN/increment PC by 2 if not equal
        {
        case 0x0000:
            if (V0 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0100:
            if (V1 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0200:
            if (V2 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0300:
            if (V3 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0400:
            if (V4 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0500:
            if (V5 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0600:
            if (V6 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0700:
            if (V7 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0800:
            if (V8 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0900:
            if (V9 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0A00:
            if (VA != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0B00:
            if (VB != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0C00:
            if (VC != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0D00:
            if (VD != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0E00:
            if (VE != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0F00:
            if (VF != nibThree + nibFour){
                PC += 2;
            }
            break;
        default:
            break;
        }
    case 0x5000:
        switch (nibTwo)                                                     // Opcode 5XY0 - Skip the next instruction if VX -= VY/increment PC by 2 if equal
        {
        case 0x0000:
            switch (nibThree)
            {
            case 0x0000:
                if (V0 == V0){
                    PC += 2;
                }
                break;
            case 0x0010:
                if (V0 == V1){
                    PC += 2;
                }
                break;
            case 0x0020:
                if (V0 == V2){
                    PC += 2;
                }
                break;
            case 0x0030:
                if (V0 == V3){
                    PC += 2;
                }
                break;
            case 0x0040:
                if (V0 == V4){
                    PC += 2;
                }
                break;
            case 0x0050:
                if (V0 == V5){
                    PC += 2;
                }
                break;
            case 0x0060:
                if (V0 == V6){
                    PC += 2;
                }
                break;
            case 0x0070:
                if (V0 == V7){
                    PC += 2;
                }
                break;
            case 0x0080:
                if (V0 == V8){
                    PC += 2;
                }
                break;
            case 0x0090:
                if (V0 == V9){
                    PC += 2;
                }
                break;
            case 0x00A0:
                if (V0 == VA){
                    PC += 2;
                }
                break;
            case 0x00B0:
                if (V0 == VB){
                    PC += 2;
                }
                break;
            case 0x00C0:
                if (V0 == VC){
                    PC += 2;
                }
                break;
            case 0x00D0:
                if (V0 == VD){
                    PC += 2;
                }
                break;
            case 0x00E0:
                if (V0 == VE){
                    PC += 2;
                }
                break;
            case 0x00F0:
                if (V0 == VF){
                    PC += 2;
                }
                break;
            default:
                break;
            }
            break;
        case 0x0100:
            switch (nibThree)
            {
            case 0x0000:
                if (V1 == V0){
                    PC += 2;
                }
                break;
            case 0x0010:
                if (V1 == V1){
                    PC += 2;
                }
                break;
            case 0x0020:
                if (V1 == V2){
                    PC += 2;
                }
                break;
            case 0x0030:
                if (V1 == V3){
                    PC += 2;
                }
                break;
            case 0x0040:
                if (V1 == V4){
                    PC += 2;
                }
                break;
            case 0x0050:
                if (V1 == V5){
                    PC += 2;
                }
                break;
            case 0x0060:
                if (V1 == V6){
                    PC += 2;
                }
                break;
            case 0x0070:
                if (V1 == V7){
                    PC += 2;
                }
                break;
            case 0x0080:
                if (V1 == V8){
                    PC += 2;
                }
                break;
            case 0x0090:
                if (V1 == V9){
                    PC += 2;
                }
                break;
            case 0x00A0:
                if (V1 == VA){
                    PC += 2;
                }
                break;
            case 0x00B0:
                if (V1 == VB){
                    PC += 2;
                }
                break;
            case 0x00C0:
                if (V1 == VC){
                    PC += 2;
                }
                break;
            case 0x00D0:
                if (V1 == VD){
                    PC += 2;
                }
                break;
            case 0x00E0:
                if (V1 == VE){
                    PC += 2;
                }
                break;
            case 0x00F0:
                if (V1 == VF){
                    PC += 2;
                }
                break;
            default:
                break;
            }
            break;
        case 0x0200:
            if (V2 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0300:
            if (V3 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0400:
            if (V4 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0500:
            if (V5 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0600:
            if (V6 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0700:
            if (V7 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0800:
            if (V8 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0900:
            if (V9 != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0A00:
            if (VA != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0B00:
            if (VB != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0C00:
            if (VC != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0D00:
            if (VD != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0E00:
            if (VE != nibThree + nibFour){
                PC += 2;
            }
            break;
        case 0x0F00:
            if (VF != nibThree + nibFour){
                PC += 2;
            }
            break;
        default:
            cout << "Code Error!";
            exit(1);
            break;
        }
    default:
        cout << "Code Error!";
        exit(1);
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////int main/////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

int main() {
    uint16_t opCode = 0;

    string filename = "";
    cout << "Specify Chip-8 ROM file name: ";
    cin >> filename;

    fileReadToMemory(filename);

    while(1){
        opCode = fetch();
        decode_Execute(opCode);
    }

    return 0;
}