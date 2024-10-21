#include "emulator.h"
#include "machine.h"
#include <stdio.h>
#include <SDL2/SDL.h>

//What needs to go here:
//Setup ports to bind them to CPUIO memory 
//Initialize SDL and create the window
//in main execute the instructions while emulating CPU and handling SDL
void init_ports(State8080 *state){
        state->input[INP1] = read_input1;
        state->input[INP2] = read_input2; 
        state->input[SHFT_IN] = read_shft;

        state->output[SHFTAMNT] = wr_shft; 
        state->output[SHFT_DATA] = wr_shft_data; 
        
        state->output[SOUND1] = wr_snd1; 
        state->output[SOUND2] = wr_snd2; 

        state->output[WATCHDOG] = wr_watchdog; 
}

//init sdl
bool init_SDL(void){
        return SDL_Init(SDL_INIT_AUDIO || SDL_INIT_VIDEO) >= 0;
}

//create SDL Window
SDL_Window *create(void){
        SDL_Window *screen = SDL_CreateWindow("Invaders_Window", 
                                                SDL_WINDOWPOS_CENTERED, 
                                                SDL_WINDOWPOS_CENTERED,  
                                                WIDTH, 
                                                LENGTH, 
                                                SDL_WINDOW_SHOWN);
        return screen; 
}


int main(void){
        State8080* state = init8080();

        //load file into memory
        if(!ReadIntoMemAt(state, "../space_invaders/invaders.h", 0)
           || !ReadIntoMemAt(state, "../space_invaders/invaders.g", 0x800)
           || !ReadIntoMemAt(state, "../space_invaders/invaders.f", 0x1000)
           || !ReadIntoMemAt(state, "../space_invaders/invaders.e", 0x1800)){
                fprintf(stderr, "ERROR LOADING FILE\n");      
                return 1; 
        }
        
        //init ports
        init_ports(&state);
        //start sdl
        if(!init_SDL()){
                fprintf(stderr, "ERROR INITIALIZING SDL: %s\n", SDL_GetError());
                return 1; 
        }
        
        //create sdl window
        SDL_Window *window = create();
        if(!window){
                fprintf(stderr, "ERROR CREATING SDL WINDOW: %s\n", SDL_GetError());
                return 1; 
        }

        //create sdl surface
        SDL_Surface *surface = SDL_GetWindowSurface(window);
        if(!surface){
                fprintf(stderr, "ERROR CREATING SDL SURFACE: %s\n", SDL_GetError());
                return 1; 
        }
        
        //setup sdl audio
        //
        //COME ABACKKKK
        
        //actually emulate CPU
        while(!state.exit){
                //DRAW DISPLAY HERE

                state.exit = !input_handler(); 
                uint64_t ticks = SDL_GetTicks64(); 
                
                while(state8080.cycle_total < (VBLANK_RATE/2)){
                        Emulate8080op(state);
                }
                genInterrupt(state, 1);

                while(state8080.cycle_total < (VBLANK_RATE)){
                        Emulate8080op(state);
                }
                genInterrupt(state, 2);

        }

}



