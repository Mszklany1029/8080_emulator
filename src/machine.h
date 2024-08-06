

#include "emulator.h"
#include <SDL2/SDL.h>

/*
 * The Space Invaders machine should handle the ports,
 * the display, 
 * the init code, 
 * the timer and interrupts,
 * and start the whole emulation cycle
 */ 

//IO ports

uint8_t read_input1(void);
uint8_t read_input2(void);
uint8_t read_shft(void);
void wr_shft_amt(uint8_t data);
void wr_snd1(uint8_t data);
void wr_shft_data(uint8_t data);
void wr_snd2(uint8_t data);
void wr_watchdog(uint8_t data);



