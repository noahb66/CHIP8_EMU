#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "chip8.h"

// This file is for recreating the Chip 8 system. It includes all "parts" and all of the opcode instructions.
// It will get called through the main.c file

#define debug_print(fmt, ...)                         \
    do {                                              \
        if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); \
    } while (0)

int DEBUG = 1;
extern int errno;

// Font set
unsigned char fontset[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
        0x20, 0x60, 0x20, 0x20, 0x70,  // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
        0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
        0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
        0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
        0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
        0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

//memory
unsigned char memory[4096] = {0};

/* -16 general purpose 8-bit registers, sometimes called Vx where "x" is a hexadecimal digit -
   -A char is 8-bits, so we store 16 of them */
unsigned char V[16] = {0};

// There is a special 16-bit register called I that is used to store memory addresses
unsigned short I = 0;

// The PC is the program counter which will point to the current instruction in memory. It is a pseudo-register and is 16 bits
unsigned short pc = 0x200;

// The stack pointer pseudo-register will point to the top of the stack. It is 8 bits
unsigned char sp = 0;

// The stack: An array of 16 16-bit values, used to store the address that the interpreter should return when finished with a subroutine
unsigned short stack[16] = {0};

// keypad
unsigned char keypad[16] = {0};

// Display: 64x32 monochrome display
unsigned char display[64 * 32] = {0};

// delay timer
unsigned char delayTimer = 0;

// Sound Timer
unsigned char soundTimer = 0;

//update display flag
unsigned char draw_flag = 0;

// Play a sound flag
unsigned char sound_flag = 0;



// Initializing the CPU. We will be loading the font set into memory
void initialize_cpu(void) {
    srand((unsigned int)time(NULL));

    // loading font set into memory
    memcpy(memory, fontset, sizeof (fontset));
}

// Needs further studying tbh
int load_rom(char * filename) {
    FILE* fp = fopen(filename, "rb");

    if (fp == NULL) return errno;

    struct stat st;
    stat(filename, &st);
    size_t fsize = st.st_size;

    size_t bytes_read = fread(memory + 0x200, 1, sizeof(memory) - 0x200, fp);

    printf("file size: %zu\nbytes read: %zu\n", fsize, bytes_read);

    if (bytes_read != fsize) {
        return -1;
    }

    fclose(fp);

    return 0;
}

void emulate_cycle(void) {
    draw_flag = 0;
    sound_flag = 0;

    // fetching the opcode
    unsigned short opcode = memory[pc] << 8 | memory[pc + 1];

    // increment the PC before execution
    //pc += 2;

    // Vx register.
    unsigned short x = (opcode & 0x0F00) >> 8;

    // Vy  register
    unsigned short y = (opcode & 0x00F0) >> 4;

    // For summing registers on case 0x8000:0x4000
    //unsigned short sum = 0;

    /*
     decode the opcode:
     Using bitwise & to mask the opcode and get the first position number using 0xF000 where the F designates the first number/letter.
     Nested switches will be used if further checking is required so for the first case 0x0000 we will need to get the last 2 using 0x00FF
    */
    switch(opcode & 0xF000)
    {
        // The first few that start with 00 will need a nested switch to allow both to work.
        case 0x0000:
            switch(opcode & 0x00FF) {
                case 0x00E0:
                    // clear screen
                    debug_print("[OK] 0x%X: 00E0\n", opcode);
                    memset(display, 0, sizeof display);
                    pc += 2;
                    break;
                case 0x00EE:
                    // return from subroutine
                    debug_print("[OK] 0x%X: 00EE\n", opcode);
                    pc = stack[sp];
                    sp--;
                    pc += 2;
                    break;
                default:
                    debug_print("[FAILED] Unknown opcode: 0x%X\n", opcode);
                    break;
            }
            break;
        case 0x1000:
            // 1NNN jump to location nnn.
            debug_print("[OK] 0x%X: 1NNN\n", opcode);
            pc = opcode & 0x0FFF;
            break;
        case 0x2000:
            // 2NNN: call subroutine at nnn.
            debug_print("[OK] 0x%X: 2NNN\n", opcode);
            ++sp;
            stack[sp] = pc;
            pc = opcode & 0x0FFF;
            break;
        case 0x3000:
            // Skip next instruction if Vx = kk.
            debug_print("[OK] 0x%X: 3XNN\n", opcode);

            if (V[x] == (opcode & 0x00FF)) {
                pc += 2;
            }
            pc += 2;
            break;
        case 0x4000:
            // 4xkk - SNE Vx, byte
            // skip next instruction if Vx != kk.
            debug_print("[OK] 0x%X: 4XNN\n", opcode);

            if (V[x] != (opcode & 0x00FF)) {
                pc += 2;
            }
            pc += 2;
            break;
        case 0x5000:
            // 5xy0 - SE Vx, Vy
            // skip next instruction if Vx = Vy
            debug_print("[OK] 0x%X: 5XY0\n", opcode);

            if (V[x] == V[y]) {
                pc += 2;
            }
            pc += 2;
            break;
        case 0x6000:
            // 6xkk - LD Vx, byte
            // set Vx = kk
            debug_print("[OK] 0x%X: 6XNN\n", opcode);
            V[x] = opcode & 0x00FF;
            pc += 2;
            break;
        case 0x7000:
            debug_print("[OK] 0x%X: 7XNN\n", opcode);
            V[x] += opcode & 0x00FF;
            pc += 2;
            break;

        case 0x8000:
            //adding nested switch for cases 8xy0-E
            switch(opcode & 0x000F) {
                case 0x0000:
                    debug_print("[OK] 0x%X: 8XY0\n", opcode);
                    // set Vx = Vy
                    V[x] = V[y];
                    pc += 2;
                    break;
                case 0x0001:
                    // Set Vx = Vx OR Vy
                    debug_print("[OK] 0x%X: 8XY1\n", opcode);
                    V[x] |= V[y];
                    pc += 2;
                    break;
                case 0x0002:
                    // Set Vx = Vx AND Vy
                    debug_print("[OK] 0x%X: 8XY2\n", opcode);
                    V[x] &= V[y];
                    pc += 2;
                    break;
                case 0x0003:
                    // Set Vx = Vx XOR Vy
                    debug_print("[OK] 0x%X: 8XY3\n", opcode);
                    V[x] ^= V[y];
                    pc += 2;
                    break;
                case 0x0004:
                    // set Vx = Vx + Vy and VF = carry
                    //The values of Vx and Vy are added together. If the result is greater than 8 bits (i.e., > 255,) VF is set to 1,
                    // otherwise 0. Only the lowest 8 bits of the result are kept, and stored in Vx.
                    debug_print("[OK] 0x%X: 8XY4\n", opcode);

                    if ((V[x] + V[y]) > 0xFF)
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    V[x] += V[y];

                    pc += 2;
                    break;
                case 0x0005:
                    //8xy5 - SUB Vx, Vy
                    //Set Vx = Vx - Vy, set VF = NOT borrow.
                    //If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.
                    debug_print("[OK] 0x%X: 8XY5\n", opcode);

                    if (V[x] > V[y])
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;

                    V[x] -= V[y];
                    pc += 2;
                    break;
                case 0x0006:
                    //8xy6 - SHR Vx {, Vy}
                    //Set Vx = Vx SHR 1.
                    //If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
                    debug_print("[OK] 0x%X: 8XY6\n", opcode);

                    V[0xF] = V[x] & 0x1;
                    V[x] = V[x] >> 1;

                    pc += 2;
                    break;
                case 0x0007:
                    //8xy7 - SUBN Vx, Vy
                    //Set Vx = Vy - Vx, set VF = NOT borrow.
                    //If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.
                    debug_print("[OK] 0x%X: 8XY7\n", opcode);

                    if (V[y] > V[x])
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    V[x] = V[y] - V[x];

                    pc += 2;
                    break;
                case 0x000E:
                    //8xyE - SHL Vx {, Vy}
                    //Set Vx = Vx SHL 1.
                    //If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.????

                    // Upon research it seems as though this instruction is outdated and for emulation something different is required
                    // I assigned the MSB into the VF register and shifted Vx left 1 which is the equivalent of multiplication by 2
                    debug_print("[OK] 0x%X: 8XYE\n", opcode);

                    V[0xF] = (V[x] >> 7) & 0x1;
                    V[x] <<= 1;
                    pc += 2;
                    break;
                default:
                    printf("[FAILED] Unknown op: 0x%X\n", opcode);
                    break;
            }
            break;

                case 0x9000:
                    //Skip next instruction if Vx != Vy.
                    //The values of Vx and Vy are compared, and if they are not equal, the program counter is increased by 2.
                    debug_print("[OK] 0x%X: 9XY0\n", opcode);

                    if (V[x] != V[y])
                        pc += 2;
                    pc += 2;
                    break;
                case 0xA000:
                    //Annn - LD I, addr
                    //Set I = nnn.
                    //The value of register I is set to nnn.
                    debug_print("[OK] 0x%X: ANNN\n", opcode);

                    I = opcode & 0x0FFF;
                    pc += 2;
                    break;
                case 0xB000:
                    //Bnnn - JP V0, addr
                    //Jump to location nnn + V0.
                    //The program counter is set to nnn plus the value of V0.
                    debug_print("[OK] 0x%X: BNNN\n", opcode);

                    pc = V[0] + (opcode & 0x0FFF);
                    break;
                case 0xC000:
                    //Cxkk - RND Vx, byte
                    //Set Vx = random byte AND kk.
                    //The interpreter generates a random number from 0 to 255, which is then ANDed with the value kk.
                    // The results are stored in Vx.
                    debug_print("[OK] 0x%X: CXNN\n", opcode);
                    V[x] = (rand() % 256) & (opcode & 0x00FF);
                    pc += 2;
                    break;
                case 0xD000:
                {
                    /* Dxyn - DRW Vx, Vy, nibble
                       Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
                       The interpreter reads n bytes from memory, starting at the address stored in I.
                       These bytes are then displayed as sprites on screen at coordinates (Vx, Vy).
                       Sprites are XORed onto the existing screen. If this causes any pixels to be erased,
                       VF is set to 1, otherwise it is set to 0. If the sprite is positioned so part of it is
                       outside the coordinates of the display, it wraps around to the opposite side of the screen.
                       See instruction 8xy3 for more information on XOR, and section 2.4, Display, for more information on
                       the Chip-8 screen and sprites. */
                    debug_print("[OK] 0x%X: DXYN\n", opcode);

                    unsigned short height = opcode & 0x000F;
                    unsigned short pixel;

                    unsigned int xCoord = V[x] % 64;
                    unsigned int yCoords = V[y] % 32;

                    // setting collision flag to 0
                    V[0xF] = 0;

                    // iterating over n rows
                    for (int yline = 0; yline < height; yline++) {
                        // getting pixel value for memory location starting at I
                        pixel = memory[I + yline];

                        // for each of the 8 pixels in this sprite row
                        for (int xline = 0; xline < 8; xline++) {

                            unsigned int spritePixel = pixel & (0x80 >> xline);
                            unsigned char* screenPixel = &display[(yCoords + yline) * 64 + (xCoord + xline)];

                            if (spritePixel != 0) {
                                if (*screenPixel == 1) {
                                    V[0xF] = 1;
                                }
                                // set the pixel value using a XOR
                                *screenPixel ^= 0xFFFFFFFF;
                            }
                        }
                    }
                    draw_flag = 1;
                    pc += 2;
                }
                    break;

                case 0xE000:
                    switch (opcode & 0x00FF) {
                        case 0x009E:
                            //Ex9E - SKP Vx
                            //Skip next instruction if key with the value of Vx is pressed.
                            //Checks the keyboard, and if the key corresponding to the value of Vx is currently in the down position, PC is increased by 2.
                            debug_print("[OK] 0x%X: EX9E\n", opcode);

                            if (keypad[V[x]]) {
                                pc += 2;
                            }
                            pc += 2;
                            break;
                        case 0x0A1:
                            //ExA1 - SKNP Vx
                            //Skip next instruction if key with the value of Vx is not pressed.
                            //Checks the keyboard, and if the key corresponding to the value of Vx is currently in the up position, PC is increased by 2.
                            debug_print("[OK] 0x%X: EXA1\n", opcode);

                            if (!keypad[V[x]])
                                pc += 2;
                            pc += 2;
                            break;
                        default:
                            printf("[FAILED] Unknown op: 0x%X", opcode);
                            break;
                    }
                    break;

                case 0xF000:
                    switch(opcode & 0x00FF)
                    {
                        case 0x0007:
                            //Fx07 - LD Vx, DT
                            //Set Vx = delay timer value.
                            //The value of DT is placed into Vx.
                            debug_print("[OK] 0x%X: FX07\n", opcode);
                            V[x] = delayTimer;

                            pc += 2;
                            break;
                        case 0x000A:
                            //Fx0A - LD Vx, K
                            //Wait for a key press, store the value of the key in Vx.
                            //All execution stops until a key is pressed, then the value of that key is stored in Vx.
                            debug_print("[OK] 0x%X: FX0A\n", opcode);

                            for (int i = 0; i < 16; i++) {
                                if (keypad[i]) {
                                    V[x] = i;
                                    pc += 2;
                                    break;
                                }
                            }
                            break;
                        case 0x0015:
                            //Fx15 - LD DT, Vx
                            //Set delay timer = Vx.
                            //DT is set equal to the value of Vx.
                            debug_print("[OK] 0x%X: FX15\n", opcode);
                            delayTimer = V[x];
                            pc += 2;
                            break;
                        case 0x0018:
                            //Fx18 - LD ST, Vx
                            //Set sound timer = Vx.
                            //ST is set equal to the value of Vx.
                            debug_print("[OK] 0x%X: FX18\n", opcode);
                            soundTimer = V[x];
                            pc += 2;
                            break;
                        case 0x001E:
                            //Fx1E - ADD I, Vx
                            //Set I = I + Vx.
                            //The values of I and Vx are added, and the results are stored in I.
                            debug_print("[OK] 0x%X: FX1E\n", opcode);
                            I = I + V[x];
                            pc += 2;
                            break;
                        case 0x0029:
                            //Fx29 - LD F, Vx
                            //Set I = location of sprite for digit Vx.
                            //The value of I is set to the location for the hexadecimal sprite corresponding to the value of Vx.
                            debug_print("[OK] 0x%X: FX29\n", opcode);
                            I = V[x] * 5;
                            pc += 2;
                            break;
                        case 0x0033:
                            //Fx33 - LD B, Vx
                            //Store BCD representation of Vx in memory locations I, I+1, and I+2.
                            //The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at location in I,
                            // the tens digit at location I+1, and the ones digit at location I+2.
                            debug_print("[OK] 0x%X: FX33\n", opcode);

                            memory[I] = (V[x] % 1000) / 100; // getting hundreds place
                            memory[I + 1] = (V[x] % 100) / 10; // getting the tens place
                            memory[I + 2] = V[x] % 10; // getting the ones place

                            pc += 2;
                            break;
                        case 0x0055:
                            //Fx55 - LD [I], Vx
                            //Store registers V0 through Vx in memory starting at location I.
                            //The interpreter copies the values of registers V0 through Vx into memory, starting at the address in I.
                            debug_print("[OK] 0x%X: FX55\n", opcode);

                            for (int i = 0; i <= x; i++)
                                memory[I + i] = V[i];
                            pc += 2;
                            break;
                        case 0x0065:
                            //Fx65 - LD Vx, [I]
                            //Read registers V0 through Vx from memory starting at location I.
                            //The interpreter reads values from memory starting at location I into registers V0 through Vx.
                            debug_print("[OK] 0x%X: FX65\n", opcode);

                            for (int i = 0; i<= x; i++)
                                V[i] = memory[I + i];

                            pc += 2;
                            break;

                    default:
                        printf("[FAILED] Unknown op: 0x%X\n", opcode);
                        break;

        }
        break;
    }

    // updating delay timer and sound timer
    if (delayTimer > 0)
        --delayTimer;

    if (soundTimer > 0 ) {
        sound_flag = 1;
        printf("BEEP!\n");
        --soundTimer;
    }
}
