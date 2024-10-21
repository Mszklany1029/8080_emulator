#ifndef MACHINE_H
#define MACHINE_H

#include "emulator.h"
#include <SDL2/SDL.h>

/*
 * The Space Invaders machine should handle the ports,
 * the display, 
 * the init code, 
 * the timer and interrupts,
 * and start the whole emulation cycle
 */ 

#define WIDTH 224
#define LENGTH 256
#define SCREEN_BYTES 7168
#define SCALE 1

#define REFRESH_RATE 60
#define CLOCK_SPEED 2000000
#define VBLANK_RATE (CLOCK_SPEED/REFRESH_RATE)
#define VRAM 0x2400


//IO ports

//READ PORTS
#define INP0 0
#define INP1 1
#define INP2 2
#define SHFT_IN 3

//WRITE PORTS
#define SHFTAMNT 2
#define SOUND1 3
#define SHFT_DATA 4
#define SOUND2 5
#define WATCHDOG 6

uint8_t read_input1(void);
uint8_t read_input2(void);
uint8_t read_shft(void);
void wr_shft_amt(uint8_t data);
void wr_snd1(uint8_t data);
void wr_shft_data(uint8_t data);
void wr_snd2(uint8_t data);
void wr_watchdog(uint8_t data);

void draw_display(SDL_Window *window, SDL_Surface *surface, const State8080 *state);

int input_handler(void); //MAYBE RETURN BOOL INStEAD?
//We need functions to draw the display
//initialize and quit the audio
//and handle the input

#endif
