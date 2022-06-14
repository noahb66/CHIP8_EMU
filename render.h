//
// Created by Noah Beal on 6/10/22.
//

#ifndef CHIP8_EMU_RENDER_H
#define CHIP8_EMU_RENDER_H

void initialize_display(void);
void draw(unsigned char* display);
void sdl_ehandler(unsigned char* keypad);
int should_quit();
void stop_display();

#endif //CHIP8_EMU_RENDER_H
