#include "machine.h"

typedef enum{
        CREDIT = 1; 
        START_P2 = (1 << 1);
        START_P1 = (1 << 2);
        P1_SHOT = (1 << 4);
        P1_LEFT = (1 << 5);
        P1_RIGHT = (1 << 6);

        TILT = (1 << 2);
        P2_SHOT = (1 << 4);
        P2_LEFT = (1 << 5);
        P2_RIGHT = (1 << 6);

        UFO = 1; 
        SHOT = (1 << 1);
        FLASH = (1 << 2);
        INVADER_DIE = (1 << 3); 
        EXTRA_LIFE = (1 << 4); 

        FLEET_MOV_1 = 1; 
        FLEET_MOV_2 = (1 << 1);
        FLEET_MOV_3 = (1 << 2); 
        FLEET_MOV_4 = (1 << 3);
        UFO_HIT = (1 << 4);

}PORT_BITS;


static uint8_t in_port1 = 0;
static uint8_t snd_port1 = 0; 
static uint8_t in_port2 = 0; 
static uint8_t snd_port2 = 0; 
static uint8_t shift_low = 0;
static uint8_t shift_hi = 0;
static uint8_t shift_offset = 0; 
//return 1 from IN 0 and return 0 from IN 1 for ATTRACT MODE?

static SDL_Event e; 

uint8_t read_input1(void){
        return in_port1;
}

uint8_t read_input2(void){
        return in_port2;
}

uint8_t read_shft(void){
        uint16_t temp = (shift_hi << 8) | shift_low; 
        uint8_t a = ((temp >> (8 - shift_offset)) & 0xff);
        return a; 
        //COME BACK
}

void wr_shft_amt(uint8_t data){
        shift_offset = data & 0x7;
}

void wr_shft_data(uint8_t data){
        shift_low = shift_hi;
        shift_hi = data; 
}

static void pixel_set(SDL_Surface *surface, int x_coord, int y_coord, long color){
        uint32_t *pixels = (uint32_t*) surface->pixels;
        pixels[y_coord * (surface->w) + x_coord] = color; 

}




void draw_display(SDL_Window *window, SDL_Surface *surface, const State8080 *state) {
    for (int i = 0; i < SCREEN_BYTES; i++) {
        uint8_t byte = state->memory[VRAM + i];

        int y = (((LENGTH * SCALE) - 1) - ((i % 32) * (8 * (SCALE))));
        int x = ((i / 32)) * SCALE;

        for (int k = 0; k < 8; k++) {
            int tmp_y = y - (k * (SCALE));

            for (int ys = 0; ys < SCALE; ys++) {
                for (int xs = 0; xs < SCALE; xs++) {
                    int final_x = x + xs;
                    int final_y = tmp_y - ys;
                    long color;

                    if (final_y >= 0 && final_y < 32 * SCALE) {
                        color = 0xFFFFFF;
                    }else if (final_y >= 32 * SCALE && final_y < 64 * SCALE) {
                        color = 0xFF0000;
                    } else if (final_y >= 64 * SCALE && final_y < 184 * SCALE) {
                        color = 0xFFFFFF;
                    } else if (final_y >= 184 * SCALE && final_y < 240 * SCALE) {
                        color = 0x00FF00;
                    } else if (final_y >= 240 * SCALE && final_y < 256 * SCALE && final_x >= 0 && final_x < 16 * SCALE) {
                        color = 0xFFFFFF;
                    } else if (final_y >= 240 * SCALE && final_y < 256 * SCALE && final_x >= 16 * SCALE && final_x < 134 * SCALE) {
                        color = 0x00FF00;
                    } else if (final_y >= 240 * SCALE && final_y < 256 * SCALE && final_x >= 134 * SCALE && final_x < 224 * SCALE) {
                        color = 0xFFFFFF;
                    }

                    if (byte & (1 << k)) {
                        _set_pixel(surface, final_x, final_y, color);
                    } else {
                        _set_pixel(surface, final_x, final_y, 0x000000);
                    }
                }
            }
        }
    }

    SDL_UpdateWindowSurface(window);
}



int input_handler(void){
        while(SDL_PollEvent(&e)){
                SDL_Keycode key = e.key.keysym.sym;
                if(e.type == SDL_QUIT){
                        return 0;
                }else if(e.type == SDL_KEYDOWN){
                        switch(key){
                                case SDLK_c:
                                        in_port1 |= CREDIT;
                                        break; 
                                case SDLK_1: 
                                        in_port1 |= START_P1;
                                        break; 
                                case SDLK_2: 
                                        in_port1 |= START_P2;
                                        break;
                                case SDLK_SPACE: 
                                        in_port1 |= P1_SHOT;
                                        in_port2 |= P2_SHOT;
                                        break;
                                case SDLK_a:
                                        in_port1 |= P1_LEFT;
                                        in_port2 |= P2_LEFT;
                                        break;
                                case SDLK_d:
                                        in_port1 |= P1_RIGHT;
                                        in_port2 |= P2_RIGHT;
                                        break; 
                                case SDLK_t: 
                                        in_port2 |= TILT;
                                        break;
                                default: 
                                        break;
                        }
                }else if(e.type == SDL_KEYUP){
                        switch(key){
                                case SDLK_c:
                                        in_port1 &= ~CREDIT; 
                                        break;
                                case SDLK_1:
                                        in_port1 &= ~START_P1;
                                        break;
                                case SDLK_2:
                                        in_port2 &= ~START_P2;
                                        break; 
                                case SDLK_SPACE: 
                                        in_port1 &= ~P1_SHOT;
                                        in_port2 &= ~P2_SHOT; 
                                        break; 
                                case SDLK_a:
                                        in_port1 &= ~P1_LEFT; 
                                        in_port2 &= ~P2_LEFT;
                                        break; 
                                case SDLK_d: 
                                        in_port1 &= ~P1_RIGHT;
                                        in_port2 &= ~P2_RIGHT;
                                        break;
                                case SDLK_t:
                                        in_port2 &= ~TILT;
                                        break;
                                default: 
                                        break; 
                                }
                        }
                }
        }
        return 1; 
}
