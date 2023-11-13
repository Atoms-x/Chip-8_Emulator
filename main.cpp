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

        uint16_t byteOne = MEMORY[PC];
        uint16_t byteTwo = MEMORY[PC+1];

        byteOne = byteOne << 8;

        uint16_t opCode = byteOne + byteTwo;

        PC += 2;

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
    uint16_t nibOne;
    uint16_t nibTwo;
    uint16_t nibThree;
    uint16_t nibFour;

    nibOne = opCode & 0xF000;
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
                    cout << "NEED TO WRITE: Clears the Screen"; // Opcode 00E0 - Clears the screen
                    break;
                
                case 0x000E:
                    cout << "NEED TO WRITE: Returns from a subroutine"; // Opcode 00EE - Returns from a subroutine
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
            cout << "This is the 0NNN Opcode"; // Opcode 0NNN - Call machine code routine at address NNN
            exit(1);
            break;
        }
        break;
    case 0x1000:
        PC = (nibTwo + nibThree + nibFour); // OpCode 1NNN - Set PC to NNN/Jump to address NNN
        break;
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