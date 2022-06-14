//
// Created by Noah Beal on 5/25/22.
//

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "chip8.h"
#include "render.h"



int main(int argc, char** argv) {
    // printing values for debugging purposes
   // printf("argc: %d\nargv: %s\n", argc, argv);

    if (argc != 2) {
        printf("usage: emulator rom.ch8\n");
        return 1;
    }


    printf("[PENDING] Initializing...\n");
    initialize_cpu();
    printf("[OK] Done!\n");

    char *rom_filename = argv[1];
    printf("[PENDING] Loading rom %s...\n", rom_filename);

    int status = load_rom(rom_filename);
    printf("status: %d\n", status);

    if (status == -1) {
        printf("[FAILED] fread() failure: the return value was not equal to the rom file size.\n");
        return 1;
    }
    else if (status != 0) {
        perror("Error while loading rom");
        return 1;
    }

    printf("[OK] Rom loaded successfully!\n");

    initialize_display();
    printf("[OK] Display successfully initialized.\n");

    while (1) {
        emulate_cycle();

        sdl_ehandler(keypad);

        if (should_quit()) {
            break;
        }
        if (draw_flag) {
            draw(display);
        }

        usleep(1500);
    }

    stop_display();
    return 0;
}