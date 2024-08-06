#include "machine.h"

uint8_t in_port1;
uint8_t shift_low;
uint8_t shift_hi;
uint8_t shift_offset; 
//return 1 from IN 0 and return 0 from IN 1 for ATTRACT MODE?
uint8_t read_input1(void){
        return in_port1;
}

uint8_t read_input2(void){
        return 0;
}

uint8_t read_shft(void){
        uint16_t temp = (shift_hi << 8) | shift_low; 
        //COME BACK
}
