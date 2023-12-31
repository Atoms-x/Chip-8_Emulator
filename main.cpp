#include <iostream>
#include <fstream>
#include <stdint.h>
#include <cstdlib>
#include <chrono>

//#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

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

uint8_t frameBuffer[256] = {};                                              // Frame buffer for storing pixel data. It is 256 to represent 64 x 32 bits, broken down into 8 bytes x 32 bits. 
                                                                            // Each byte will hold 8 bits/pixels of data. Each line will start after the eighth byte. In this way, it could be stored in the Memory for the first 0x200 of data
  
uint8_t V[16] = {};                                                         // Array representing 8-bit registers - used as variables (0x0 to 0xE) + 8-bit flag register (0xF) - also a variable, but used as a flag by some instructions
uint8_t SC8F[8] = {};                                                       // Super Chip 8 HP-48 RPL Flags

uint16_t I;                                                                 // 16-bit index register for memory addresses - Only rightmost 12 bits used since only 4096 memory available
uint16_t PC = 0x200;                                                        // Program Counter - stores currently executing address

uint16_t STACK[16] = {};                                                    // Call Stack for subroutines/functions, Holds 16-bit values
uint8_t SP = 0;                                                             // Stack Pointer - points to the top level of the stack

uint8_t DELAY;                                                              // 8-bit delay timer, count down at 60Hz until it reaches 0. Value can be set and read
uint8_t SOUND;                                                              // 8-bit soudn timer, count down at 60Hz until it reaches 0. Beep is played when value is non-zero

const double HERTZ = 0.01667;                                               // 60 ticks per second

chrono::duration<double> ELAPSEDDELAYTIME;                                  // Elapsed time for Delay timer
chrono::duration<double> ELAPSEDSOUNDTIME;
auto DELAYT1 = chrono::steady_clock::now();                                 // Delay start time
auto DELAYT2 = chrono::steady_clock::now();                                 // Delay current check time

auto REFRESHT1 = chrono::steady_clock::now();
auto REFRESHT2 = chrono::steady_clock::now();
chrono::duration<double> ELAPSEDREFRESHTIME;

bool DRAWCALL = false;                                                      // Only re-draw screen when a draw call has been made

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
void decode_Execute(uint16_t opCode, SDL_Renderer* renderer, SDL_Event keyPress){
    uint16_t nibOne;                                                                                // Creating four nibbles that are 16-bit so they can be switch/cased by each digit, working down what Opcode to execute
    uint16_t nibTwo;
    uint16_t nibThree;
    uint16_t nibFour;

    nibOne = opCode & 0xF000;                                                                       // Masking the nibbles with an AND operator to leave the appropriate nibble/hex digit in place for checking
    nibTwo = opCode & 0x0F00;
    nibThree = opCode & 0x00F0;
    nibFour = opCode & 0x000F;

    uint16_t hDigitX = nibTwo >> 8;
    uint16_t hDigitY = nibThree >> 4;

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
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderClear(renderer);                                                      // Opcode 00E0 - Clears the screen
                    break;
                case 0x000E:
                    PC = STACK[SP];                                                                 // Opcode 00EE - Returns from a subroutine/sets PC to NNN/addr at top of stack, then decrements the stack pointer/stack
                    SP -= 1;
                    break;
                default:
                    cout << "Code Error! 00E";
                    exit(1);
                    break;
                }
                break;
            default:
                cout << "Code Error! 00";
                exit(1);
                break;
            }
            break;
        default:
            cout << "NEED TO WRITE: 0NNN Opcode ";                                                  // Opcode 0NNN - Call machine code routine at address NNN - COULD PROBABLY BE IGNORED FOR MOST ROMS
            break;
        }
        break;
    case 0x1000:
        PC = (nibTwo + nibThree + nibFour);                                                         // Opcode 1NNN - Set PC to NNN / Jump to address NNN
        break;
    case 0x2000:
        SP += 1;                                                                                    // Opcode 2NNN - Calls subroutine at NNN / put PC onto the stack and set PC to NNN
        STACK[SP] = PC;
        PC = (nibTwo + nibThree + nibFour);
        break;
    case 0x3000:                                            
        if (V[hDigitX] == nibThree + nibFour){                                                      // Opcode 3XNN - Skip the next instruction if VX == NN / increment PC by 2 if equal
            PC += 2;
        }
        break;
    case 0x4000:                                                                   
        if (V[hDigitX] != nibThree + nibFour){                                                      // Opcode 4XNN - Skip the next instruction if VX != NN / increment PC by 2 if not equal
            PC += 2;
        }
        break;
    case 0x5000:
        if (V[hDigitX] == V[hDigitY]){                                                              // Opcode 5XY0 - Skip the next instruction if VX == VY / increment PC by 2 if equal
            PC += 2;
        }
        break;
    case 0x6000:                                  
        V[hDigitX] = nibThree + nibFour;                                                            // Opcode 6xNN - Set Vx to NN
        break;
    case 0x7000:                                                                                    // Opcode 7xNN - Add NN to VX / VX = VX + NN (Don't change the carry flag)
        if (hDigitX != 0x000F){
            V[hDigitX] = V[hDigitX] + (nibThree + nibFour); 
        }
        break;
    case 0x8000:
        switch (nibFour)                                                    
        {
        case 0x0000:                                                                                // Opcode 8XY0 - Set VX = VY
            V[hDigitX] = V[hDigitY];
            break;
        case 0x0001:                                                        
            V[hDigitX] = V[hDigitX] | V[hDigitY];                                                   // Opcode 8XY1 - Set VX = VX | VY
            break;
        case 0x0002:
            V[hDigitX] = V[hDigitX] & V[hDigitY];                                                   // Opcode 8XY2 - Set VX = VX & VY
            break;
        case 0x0003:
            V[hDigitX] = V[hDigitX] ^ V[hDigitY];                                                   // Opcode 8XY3 - Set VX = VX ^ VY
            break;
        case 0x0004:                                                  
            if((0x00FF - V[hDigitX] < V[hDigitY])){                                                 // Opcode 8XY4 - Set VX = VX + VY, if VX > 255 set VF = 1, otherwise VF = 0              
                V[hDigitX] = V[hDigitX] + V[hDigitY]; 
                V[0x000F] = 1;
            }
            else{
                V[hDigitX] = V[hDigitX] + V[hDigitY]; 
                V[0x000F] = 0;
            }                  
            break;
        case 0x0005:                                                                                // OpCode 8XY5 - If VX > VY set VF = 1, otherwise VF = 0. The VX = VX - VY
            if (V[hDigitX] >= V[hDigitY]){
                V[hDigitX] = V[hDigitX] - V[hDigitY];
                V[0x000F] = 1;
            }
            else{
                V[hDigitX] = V[hDigitX] - V[hDigitY];
                V[0x000F] = 0;
            }
            break;
        case 0x0006:
            if ((V[hDigitX] & 0x0001 == 1)){                                                        // Opcode 8XY6 - IF least-significant bit of VX is 1, VF = 1, else VF = 0, then bit shift to divide VX by 2
                V[hDigitX] = V[hDigitX] >> 1;
                V[0x000F] = 1;
            }
            else{
                V[hDigitX] = V[hDigitX] >> 1;
                V[0x000F] = 0;
            }
            break;
        case 0x0007:
            if (V[hDigitX] <= V[hDigitY]){                                                           // Opcode 8XY7 - If VX < VY set VF = 1, otherwise VF = 0. The VX = VY - VX
                V[hDigitX] = V[hDigitY] - V[hDigitX];
                V[0x000F] = 1;
            }
            else{
                V[hDigitX] = V[hDigitY] - V[hDigitX];
                V[0x000F] = 0;
            }
            break;
        case 0x000E:
            if (((V[hDigitX])>>7) & 0x0001 == 1){                                                   // Opcode 8XYE - If most-significant bit of VX is 1, VF = 1, else VF = 0, then bit shift to divide VX by 2
                V[hDigitX] = V[hDigitX] << 1;
                V[0x000F] = 1;
            }
            else{
                V[hDigitX] = V[hDigitX] << 1;
                V[0x000F] = 0;
            }
            break;
        default:
            cout << "Code Error! 8000";
            exit(1);
            break;
        }
        break;
    case 0x9000:
        switch (nibFour)
        {
        case 0x0000:                                                                                // Opcode 9XY0 - If VX != VY skip next instruction / increment PC by 2 if not equal
            if(V[hDigitX] != V[hDigitY]){
                PC += 2;
            }
            break;
        default:
            cout << "Code Error! 9000";
            exit(1);
            break;
        }
        break;
    case 0xA000:
        I = (nibTwo + nibThree + nibFour);                                                          // Opcode ANNN - Set I to address NNN
        break;
    case 0xB000:
        PC = (nibTwo + nibThree + nibFour) + V[0x0];                                                // Opcode BNNN - Set PC to address NNN + V0
        break;
    case 0xC000:                                                                                    // Opcode CXNN - Set VX = Random nubmer between 0 and 255 & NN
        srand(time(0));
        V[hDigitX] = (rand() % 255) & (nibThree + nibFour);
        break;
    case 0xD000:{                                                                                   // Opcode DXYN - Display
        uint8_t N = nibFour;                                                                        // Height of sprite in memory / number of memory locations to check
        uint8_t xCord = V[hDigitX] % 64;                                                            // X-coordinate of the start of a sprite line. Modulus is because sprites will wrap
        uint8_t yCord = V[hDigitY] % 32;                                                            // Y-coordinate of the start of a sprite line. Modulus is because sprites will wrap
        uint16_t sprMemLoc = I;                                                                     // Memory location start for sprite
        uint8_t sprite = 0;

        for (int k=0; k < N; k++){
            sprite = MEMORY[sprMemLoc + k];                                                         // This is the sprite position within memory, starting with the top of the sprite

            if (xCord % 8 != 0){
                uint8_t spriteLeft = sprite >> (xCord % 8);                                         // This is the sprite bit shifted so that it fits properly into the left most pixel byte, since the sprite crosses over two bytes of pixel info
                uint8_t spriteRight = sprite << (8 - (xCord % 8));                                  // This is the sprite bit shifted so that it fits properly into the right most pixel byte, since the sprite crosses over two bytes of pixel info
                uint8_t pixelLeft = frameBuffer[(xCord/8) + (yCord * 8)];                           // This is the pixel byte position on the left within the frame buffer to XOR with the spriteLeft
                uint8_t pixelRight = frameBuffer[((xCord/8)+1) + (yCord * 8)];                      // This is the pixel byte position on the right within the frame buffer to XOR with the spriteRight

                for (int m=0; m < 8; m++){                                                          
                    if ((((spriteLeft >> m) & 0x0001) & ((pixelLeft >> m) & 0x0001)) == 1){         // Checks if there is a collision from overlapping pixels
                        V[0x000F] = 1;
                    }
                    if ((((spriteRight >> m) & 0x0001) & ((pixelRight >> m) & 0x0001)) == 1){       // Checks if there is a collision from overlapping pixels
                        V[0x000F] = 1;
                    }
                }
                frameBuffer[(xCord/8) + (yCord * 8)] = pixelLeft ^ spriteLeft;                      // XORs the sprite with the frame buffer pixel
                frameBuffer[((xCord/8)+1) + (yCord * 8)] = pixelRight ^ spriteRight;                // XORs the sprite with the frame buffer pixel
            }
            else{
                uint8_t pixel = frameBuffer[(xCord/8) + (yCord * 8)];

                for (int m=0; m < 8; m++){
                    if ((((sprite >> m) & 0x0001) & ((pixel >> m) & 0x0001)) == 1){
                        V[0x000F] = 1;
                    }
                }
                frameBuffer[(xCord/8) + (yCord * 8)] = pixel ^ sprite;
            }
            yCord++;
        }
        DRAWCALL = true;
        REFRESHT1 = chrono::steady_clock::now();
        break;
    }
    case 0xE000:
        switch (nibThree)
        {
        case 0x0090:
            switch (nibFour)
            {
            case 0x000E:{                                                                           // Opcode EX9E - Key press check for VX
                
                uint8_t key = V[hDigitX];

                while(SDL_PollEvent(&keyPress)){
                    switch (keyPress.type)
                    {
                    case SDL_KEYDOWN:
                        switch (keyPress.key.keysym.scancode)
                        {
                        case SDL_SCANCODE_1:
                            if(key == 0x1){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_2:
                            if(key == 0x2){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_3:
                            if(key == 0x3){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_4:
                            if(key == 0xC){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_Q:
                            if(key == 0x4){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_W:
                            if(key == 0x5){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_E:
                            if(key == 0x6){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_R:
                            if(key == 0xD){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_A:
                            if(key == 0x7){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_S:
                            if(key == 0x8){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_D:
                            if(key == 0x9){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_F:
                            if(key == 0xE){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_Z:
                            if(key == 0xA){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_X:
                            if(key == 0x0){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_C:
                            if(key == 0xB){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_V:
                            if(key == 0xF){
                                PC += 2;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    default:
                        break;
                    }
                }
                break;
            }                                                                             
            default:
                cout << "Code Error! EX9";
                exit(1);
                break;
            }
            break;
        case 0x00A0:
            switch (nibFour)
            {
            case 0x0001:{                                                                           // Opcode EXA1 - key press check against VX                              
                uint8_t key = V[hDigitX];
                
                while(SDL_PollEvent(&keyPress)){
                    switch (keyPress.type)
                    {
                    case SDL_KEYUP:
                        switch (keyPress.key.keysym.scancode)
                        {
                        case SDL_SCANCODE_1:
                            if(key != 0x1){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_2:
                            if(key != 0x2){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_3:
                            if(key != 0x3){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_4:
                            if(key != 0xC){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_Q:
                            if(key != 0x4){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_W:
                            if(key != 0x5){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_E:
                            if(key != 0x6){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_R:
                            if(key != 0xD){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_A:
                            if(key != 0x7){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_S:
                            if(key != 0x8){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_D:
                            if(key != 0x9){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_F:
                            if(key != 0xE){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_Z:
                            if(key != 0xA){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_X:
                            if(key != 0x0){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_C:
                            if(key != 0xB){
                                PC += 2;
                            }
                            break;
                        case SDL_SCANCODE_V:
                            if(key != 0xF){
                                PC += 2;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    default:
                        break;
                    }
                }
                break;
            }
            default:
                cout << "Code Error! EXA";
                exit(1);
                break;
            }
            break;
        default:
            cout << "Code Error! E000";
            exit(1);    
            break;
        }
        break;
    case 0xF000:
        switch (nibThree)
        {
        case 0x0000:
            switch (nibFour)
            {
            case 0x0007:{
                
                DELAYT2 = chrono::steady_clock::now();                                             // OpCode FX07 - Set VX to the value of the delay timer
                ELAPSEDDELAYTIME = DELAYT2-DELAYT1;
                
                if (ELAPSEDDELAYTIME.count() >= HERTZ){
                    if((DELAY) < (ELAPSEDDELAYTIME.count()/HERTZ)){
                        DELAY = 0;
                        DELAYT1 = DELAYT2;
                    }
                    else{
                        DELAY -= ELAPSEDDELAYTIME.count()/HERTZ;
                        DELAYT1 = DELAYT2;
                    }
                }
                V[hDigitX] = DELAY;
                break;
            }
            case 0x000A:
                cout << "NEED TO WRITE: Wait for keypress ";                                        // Opcode FX0A - Wait for keypress
                while(SDL_PollEvent(&keyPress)){
                    if(keyPress.key.state = SDL_PRESSED){
                        switch (keyPress.type)
                        {
                        case SDL_KEYDOWN:
                            switch (keyPress.key.keysym.scancode)
                            {
                            case SDL_SCANCODE_1:
                                V[hDigitX] = 0x1;
                                break;
                            case SDL_SCANCODE_2:
                                V[hDigitX] = 0x2;
                                break;
                            case SDL_SCANCODE_3:
                                V[hDigitX] = 0x3;
                                break;
                            case SDL_SCANCODE_4:
                                V[hDigitX] = 0xC;
                                break;
                            case SDL_SCANCODE_Q:
                                V[hDigitX] = 0x4;
                                break;
                            case SDL_SCANCODE_W:
                                V[hDigitX] = 0x5;
                                break;
                            case SDL_SCANCODE_E:
                                V[hDigitX] = 0x6;
                                break;
                            case SDL_SCANCODE_R:
                                V[hDigitX] = 0xD;
                                break;
                            case SDL_SCANCODE_A:
                                V[hDigitX] = 0x7;
                                break;
                            case SDL_SCANCODE_S:
                                V[hDigitX] = 0x8;
                                break;
                            case SDL_SCANCODE_D:
                                V[hDigitX] = 0x9;
                                break;
                            case SDL_SCANCODE_F:
                                V[hDigitX] = 0xE;
                                break;
                            case SDL_SCANCODE_Z:
                                V[hDigitX] = 0xA;
                                break;
                            case SDL_SCANCODE_X:
                                V[hDigitX] = 0x0;
                                break;
                            case SDL_SCANCODE_C:
                                V[hDigitX] = 0xB;
                                break;
                            case SDL_SCANCODE_V:
                                V[hDigitX] = 0xF;
                                break;
                            default:
                                break;
                            }
                            break;
                        default:
                            break;
                        }

                    }
                    else{
                        PC -= 2;
                    }
                }
                break;
            default:
                cout << "Code Error! F000";
                exit(1);
                break;
            }
            break;
        case 0x0010:
            switch (nibFour)
            {
            case 0x0005:{
                
                DELAY = V[hDigitX];                                                                 // Opcode FX15 - Set delay timer to VX
                DELAYT1 = chrono::steady_clock::now();
                                                  
                break;
            }
            case 0x0008:
                cout << "NEED TO WRITE: Set sound timer to VX ";                                    // Opcode FX18 - Set sound timer to VX
                break;
            case 0x000E:
                I = I + V[hDigitX];                                                                 // Opcode FX1E - Add values of I and VX and store in I
                if (I > 0xFFF){
                    V[0x000F] = 1;                                                                  // Extended Behavior
                }
                break;
            default:
                cout << "Code Error! F010";
                exit(1);
                break;
            }
            break;
        case 0x0020:
            switch (nibFour)
            {
            case 0x0009:
                I = hDigitX*5;                                                                      //Opcode FX29 - Set location of sprite for font character in VX
                cout << "Stored Font Call";     
                break;
            default:
                cout << "Code Error! F020";
                exit(1);
                break;
            }
            break;
        case 0x0030:{
            switch (nibFour)
            {
            case 0x0003:{
                int temp = 0;
                temp = V[hDigitX]/100;                                                              // Opcode FX33 - Some BCD stuff
                MEMORY[I] = temp;
                temp = (V[hDigitX] - (temp*100));
                MEMORY[I+1] = temp/10;
                MEMORY[I+2] = temp - ((temp/10)*10);
                break;
            }
            default:
                cout << "Code Error! F030";
                exit(1);
                break;
            }
            break;
        }
        case 0x0050:
            switch (nibFour)
            {
            case 0x0005:{
                for(int k=0; k <= hDigitX; k++){                                                      // Opcode FX55 - some regdump stuff
                    MEMORY[I+k] = V[k];
                }
                break;
            }
            default:
                cout << "Code Error! F050";
                exit(1);
                break;
            }
            break;
        case 0x0060:
            switch (nibFour)
            {
            case 0x0005:{
                for(int k=0; k <= hDigitX; k++){                                                      // Opcode FX65 - some regload stuff
                    V[k] = MEMORY[I+k];
                }
                break;
            }
            default:
                cout << "Code Error! F060";
                exit(1);
                break;
            }
            break;
        case 0x0070:
            switch (nibFour)
            {
            case 0x0005:{                                                                           // Opcode FX75 - Load V registers into HP48 style RPL flags - SUPERCHIP 8 instruction
                if (hDigitX <= 7){
                    for(int k=0; k <= hDigitX; k++){
                        SC8F[k] = V[k];
                    }
                }
                break;
            }
            default:
                cout << "Code Error! F070";
                break;
            }
            break;
        case 0x0080:
            switch (nibFour)
            {
            case 0x0005:{                                                                           // Opcode FX85 - Load HP48 style RPL flags into V registers - SUPERCHIP 8 instruction
                if (hDigitX <= 7){
                    for(int k=0; k <= hDigitX; k++){
                        V[k] = SC8F[k];
                    }
                }
                break;
            }
            default:
                cout << "Code Error! F080";
                break;
            }
            break;
        default:
            cout << "Code Error! F";
            exit(1);
            break;
        }
        break;
    default:
        cout << "Code Error! 0000";
        exit(1);
        break;
    }
}

//***************************************************************************
// void draw
//  Arguments:
//      s
//  Returns:
//      N
//  Purpose:
//      L
//***************************************************************************
void draw(SDL_Renderer* renderer){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);                                                 // Set clear screen to black
    SDL_RenderClear(renderer);                                                                      // Clear screen since not every pixel will be set or unset individually
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);                                           // Set pixel draw color to white
    for (int k=0; k < 256; k++){                                                                    // Pass through every pixel byte, from top left to bottom right as one-dimensional array abstracted into two-dimensional array
        uint8_t pixelByte = frameBuffer[k];
        uint8_t yCord = k/8;                                                                        // Screen is 64 pixels wide, meaning that a line can be made up of 8 pixel bytes to store pixel bit data as bits and save memory
        
        for (int m=0; m < 8; m++){                                                                  // Check each of the bits on each pixel byte from left to right by bit shifting to the pixel and masking with 0x0001
            if (((pixelByte >> (7-m)) & 0x1) == 1){
                SDL_RenderDrawPoint(renderer, (m+((k%8)*8)), yCord);                                // (x,y) 
            }
        }
    }
    SDL_RenderPresent(renderer);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////int main/////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argv, char** args) {
    
    int frameCheck = 0;

    SDL_Window* window = nullptr;                                                                   // Create SDL game window
    SDL_Renderer* renderer = nullptr;                                                               // Create SDL render object
    SDL_Event keyPress;

    SDL_Init(SDL_INIT_VIDEO);                                                                       // Intialize the the game window
    SDL_CreateWindowAndRenderer(64*15, 32*15, 0, &window, &renderer);                               // Give window and renderer dimensions
    SDL_RenderSetScale(renderer, 15, 15);                                                           // Set render scale for correct pixel to screen size, important for small resolution on high resolution monitors POSSIBLY MAKE ADJUSTABLE FOR VARIOUS MONITORS

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);                                                 // Set initial background color
    SDL_RenderClear(renderer);                                                                      // Clear the screen with the background color (black)

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);                                           // Set pixel draw color (white)
    
    uint16_t opCode = 0;

    string filename = "";
    cout << "Specify Chip-8 ROM file name: ";
    cin >> filename;

    fileReadToMemory(filename);

    while(1){
        opCode = fetch();
        decode_Execute(opCode, renderer, keyPress);
        
        if(DRAWCALL == true){
            
            REFRESHT2 = chrono::steady_clock::now();
            ELAPSEDREFRESHTIME = REFRESHT2 - REFRESHT1;

            while (ELAPSEDREFRESHTIME.count() <= 0.004667){
                REFRESHT2 = chrono::steady_clock::now();
                ELAPSEDREFRESHTIME = REFRESHT2 - REFRESHT1;
            }
            
            //cout << "Render: " << frameCheck << endl;
            draw(renderer);
            DRAWCALL = false;
            //frameCheck++;
        }
    }
    return 0;
}