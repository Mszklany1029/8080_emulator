#ifndef EMULATOR_H
#define EMULATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


typedef uint8_t (*input_ptr)(void);
typedef void (*output_ptr)(uint8_t);

typedef struct ConditionCodes {
	uint8_t z:1; //zero flag: set if value is 0
	uint8_t s:1; //sign flag: set if most significant bit is 1 (negative)
	uint8_t p:1; //parity flag: set if even, reset if odd
	uint8_t cy:1; //carry flag; set if high order bit is borrowed or carried
	uint8_t ac:1; //aux carry: set if carry out of bit 3 into bit 4
        
        uint8_t int_flag:1; 

	uint8_t pad:3; //COME BACK TO THIS?? 
        
} ConditionCodes;

typedef struct State8080 {
	uint8_t a;
	uint8_t b;
	uint8_t c;
	uint8_t d;
	uint8_t e;
	uint8_t h;
	uint8_t l;
	uint16_t sp;
	uint16_t pc;
	uint8_t	*memory;
	struct ConditionCodes	cc;
	uint8_t	int_enable;

        input_ptr input[0xFF];
        output_ptr output[0xFF];

        bool exit; 

        unsigned long cycle_total; 

} State8080;

int Emulate8080op(State8080* state);
void genInterrupt(State8080* state, int interrupt_num);
bool ReadIntoMemAt(State8080* state, char* filename, uint32_t offset);
State8080* init8080(void); 


#endif
