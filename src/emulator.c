#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
typedef struct ConditionCodes {
	uint8_t z:1; //zero flag: set if value is 0
	uint8_t s:1; //sign flag: set if most significant bit is 1 (negative)
	uint8_t p:1; //parity flag: set if even, reset if odd
	uint8_t cy:1; //carry flag; set if high order bit is borrowed or carried
	uint8_t ac:1; //aux carry: set if carry out of bit 3 into bit 4
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
} State8080;

void UnimplementedInstruction(State8080* state){
	//undo program counter advance
	printf("ERROR: Unimplemented instruction\n");
	exit(1);
}
int Parity(uint8_t val){
        return (val & 1) == 0;
}

/*
 * NOTE TO SELF: MIGHT HAVE TO CHANGE THINGS HERE FROM UINT8_T TO UINT16_T 
 */
static inline void e8080_adc(State8080* state, uint8_t* const dest, uint8_t src, uint8_t carry){
	uint8_t ans = *dest + src + carry;
	state -> cc.z = ((ans & 0xff) == 0);
	state -> cc.s = ((ans & 0x80) != 0);
	state -> cc.cy = (ans > 0xff);
	state -> cc.p = Parity(ans & 0xff);
	*dest = ans; 
}

static inline void e8080_sub(State8080* state, uint8_t* const dest, uint8_t src){
        uint8_t ans = *dest - src; 
        state -> cc.z = ((ans & 0xff) == 0);
        state -> cc.s = ((ans & 0x80) != 0);
        state -> cc.cy = (ans > 0xff);
        state -> cc.p = Parity(ans & 0xff);
        *dest = ans; 
}

static inline void e8080_sbb(State8080* state, uint8_t* const dest, uint8_t src, uint8_t carry){
        uint8_t ans = (*dest - src) - carry; 
        state -> cc.z = ((ans & 0xff) == 0);
        state -> cc.s = ((ans & 0x80) != 0);
        state -> cc.cy = (ans > 0xff);
        state -> cc.p = Parity(ans & 0xff);
        *dest = ans; 
}

static inline void e8080_dcr(State8080* state, uint8_t* const dest){
        uint8_t ans = *dest - 1;
        state -> cc.z = ((ans & 0xff) == 0);
        state -> cc.s = ((ans & 0x80) != 0);
        state -> cc.p = Parity(ans & 0xff);
        *dest = ans; 
}

static inline void e8080_inr(State8080* state, uint8_t* const dest){
        uint8_t ans = *dest + 1;
        state -> cc.z = ((ans & 0xff) == 0);
        state -> cc.s = ((ans & 0x80) == 0);
        state -> cc.p = Parity(ans & 0xff);
        *dest = ans; 
}

static inline uint16_t get_bc_pair(State8080* state){
        return (state->b << 8) | state -> c; 
}

static inline void set_bc_pair(State8080* state, uint16_t val){
        state -> b = val >> 8; // right shift val 8 bits to get the highorder bits into first register
        state -> c = val & 0xff; // AND 0xff allows us to get only the last 8 bits of val
}

static inline uint16_t get_de_pair(State8080* state){
        return (state -> d << 8) | state -> e; 
}

static inline void set_de_pair(State8080* state, uint16_t val){
        state -> d = val >> 8;
        state -> e = val & 0xff;
}

static inline uint16_t get_hl_pair(State8080* state){
        return (state -> h << 8) | state -> l;
}

static inline void set_hl_pair(State8080* state, uint16_t val){
        state -> h = val >> 8;
        state -> l = val & 0xff; 
}

static inline void e8080_dad(State8080* state, uint16_t addend_pair){
        uint16_t ans = get_hl_pair(state) + addend_pair;
        state -> cc.cy = (ans >> 16) & 1; 
        set_hl_pair(state, ans);
}

static inline void e8080_add(State8080* state, uint8_t val, uint8_t carry){
        uint16_t ans = (uint16_t) state -> a + (uint16_t) val + carry;
        state -> cc.z = ((ans & 0xff) == 0);
        state -> cc.s = ((ans & 0x80) != 0);
        state -> cc.cy = (ans > 0xff);
        state -> cc.p = Parity(ans & 0xff);
        state -> a = ans & 0xff;
}

static inline void jmp_ncond(State8080* state, uint8_t cond, unsigned char *opcode){
        if(0 == cond){
                state -> pc = (opcode[2] << 8) | opcode[1];
        }else{
                state -> pc += 2; 
        }
}

static inline void jmp_cond(State8080* state, uint8_t cond, unsigned char *opcode){
        if(1 == cond){
                state -> pc = (opcode[2] << 8) | opcode[1];
        }else{
                state -> pc += 2; 
        }
}

static inline void e8080_call(State8080* state){
        uint16_t ret = state -> pc+2;
        state -> memory[state -> sp-1] = (ret >> 8) & 0xff;
        state -> memory[state -> sp-2] = (ret & 0xff);
        state -> sp = state -> sp - 2;
        state -> pc = (opcode[2] << 8) | opcode[1];
}

static inline void e8080_ret(State8080* state){
        state -> pc = state->memory[state->sp] | (state -> memory[state -> sp +1] << 8);
        state -> sp += 2;
}

static inline void ncond_call(State8080* state, uint8_t cond){
        if(0 == cond){
                e8080_call(state);
        }
}

static inline void cond_call(State8080* state, uint8_t cond){
        if(1 == cond){
                e8080_call(state);
        }
}

static inline void ncond_ret(State8080* state, uint8_t cond){
        if(0 == cond){
                e8080_ret(state);
        }
}

static inline void cond_ret(State8080* state, uint8_t cond){
        if(1 == cond){
                e8080_ret(state);
        }
}

static inline void e8080_rst(State8080* state, uint16_t hl_pair, uint8_t nnn){
        state -> memory[state -> sp-1] = (hl_pair >> 8) & 0xff;
        state -> memory[state -> sp-2] = hl_pair & 0xff;
        state -> sp = state -> sp - 2;
        state -> pc = 8 * nnn;
}

static inline void e8080_rlc(State8080* state){
        uint8_t x = state -> a;
        state -> a = ((x << 1) | x >> 7); //COME BACK
        state -> cc.cy = (1 == (1 >> 7));
}

static inline void e8080_rrc(State8080* state){
        uint8_t x = state -> a; 
        state -> a = ((x & 1) << 7 | (x >> 1));
        state -> cc.cy = (1 == (x & 1));
}

static inline void e8080_ral(State8080* state){
        uint8_t x = state -> a;
        uint8_t high_order = x >> 7;
        state -> a = ((x << 1) | state -> cc.cy);
        state -> cc.cy = high_order;
}

static inline void e8080_rar(State8080* state){
        uint8_t x = state -> a;
        uint8_t low_order = x & 1; 
        state -> a = ((state -> cc.cy) << 7 | (x >> 1));
        state -> cc.cy = low_order; 
}

static inline void e8080_cma(State8080* state){
        state -> a = ~(state -> a);   
}

static inline void e8080_ana(State8080* state, uint8_t* const reg){
        uint8_t x = state-> a & *reg;
        state -> cc.z = (x == 0);
        state -> cc.s = ((x >> 7) == 1);
        state -> cc.cy = 0; //carry flag is cleared?
        state -> cc.p = Parity(x);
        state -> a = x; 
}

static inline void e8080_xra(State8080* state, uint_t* const reg){
        uint8_t x = state -> a ^ *reg;
        state -> cc.z = (x == 0);
        state -> cc.s = ((x >> 7) == 1);
        state -> cc.cy = 0;
        state -> cc.ac = 0;
        state -> cc.p = Parity(x);
        state -> a = x;
}

int Emulate8080op(State8080* state){
	unsigned char *opcode = &state->memory[state->pc];

	switch(*opcode){
		case 0x00: break; //nop
		case 0x01:	//LXI B,word
				state -> c = opcode[1];
				state -> b = opcode[2];
				state -> pc += 2;	//advance 2 bytes
				break;				   
		case 0x02: UnimplementedInstruction(state); break;
		case 0x03: set_bc_pair(state, get_bc_pair(state) + 1); break; //INX B
		case 0x04: e8080_inr(state, &state -> b); break; //INR B
                case 0x05: e8080_dcr(state, &state -> b); break;
                case 0x07: e8080_rlc(state); break; //RLC
                case 0x09: e8080_dad(state, get_bc_pair(state)); break;//DAD B
                case 0x0b: set_bc_pair(state, get_bc_pair(state) - 1); break;//DCX B
                case 0x0c: e8080_inr(state, &state -> c); break;
                case 0x0d: e8080_dcr(state, &state -> c); break;
                case 0x0f: e8080_rrc(state); break; //RRC
                case 0x13: set_de_pair(state, get_de_pair(state) + 1); break;//INX D
                case 0x14: e8080_inr(state, &state -> d); break; 
                case 0x15: e8080_dcr(state, &state -> d); break;
                case 0x17: e8080_ral(state); break;
                case 0x19: e8080_dad(state, get_de_pair(state)); break;//DAD D
                case 0x1b: set_de_pair(state, get_de_pair(state) - 1); break;//DCX D
                case 0x1c: e8080_inr(state, &state -> e); break;
                case 0x1d: e8080_dcr(state, &state -> e); break;
                case 0x1f: e8080_rar(state); break;
                case 0x23: set_hl_pair(state, get_hl_pair(state) + 1);
                case 0x24: e8080_inr(state, &state -> h); break;
                case 0x25: e8080_dcr(state, &state -> h); break;
                case 0x27: //DAA
                /*case 0x29: //DAD H
                case 0x2b: //DCX H
                case 0x2c: e8080_inr(state, &state -> l); break;*/ 
                case 0x29: e8080_dad(state, get_hl_pair(state)); break;//DAD H
                case 0x2b: set_hl_pair(state, get_hl_pair(state) - 1); 
                case 0x2c: e8080_inr(state, &state -> l); break; 
                case 0x2d: e8080_dcr(state, &state -> l); break;
                case 0x2f: e8080_cma(state); break;//CMA
                case 0x33: state -> sp = state -> sp + 1; //INX SP
                /*case 0x34: e8080_inr(state, &state -> m); break;//FIXXXXXX THESE AFFECT REGISTER PAIRS NOT SINGLE REGISTERS
                case 0x35: e8080_dcr(state, &state -> m); break;//FIXXXXXXXXX*/
                case 0x39: e8080_dad(state, state->sp);//DAD SP
                case 0x3b: state -> sp = state -> sp - 1; //DCX SP
                case 0x3c: e8080_inr(state, &state -> a); break;
                case 0x3d: e8080_dcr(state, &state -> a); break;
                case 0x3f: state -> cc.cy = ~(state->cc.cy); break;
				   /*OTHER CASES HERE*/
		case 0x41: state -> b = state -> c; break; //MOV B, C
		case 0x42: state -> b = state -> d; break; //MOV B, D
		case 0x43: state -> b = state -> e; break; //MOV B, E
			/*OTHER CASES HERE*/
		case 0x80:
			{
			   uint16_t ans = (uint16_t) state-> a + (uint16_t) state -> b;
			   state -> cc.z = ((ans & 0xff) == 0);
			   state -> cc.s = ((ans & 0x80) != 0);
			   state -> cc.cy = (ans > 0xff);
			   state -> cc.p = Parity(ans & 0xff);
			   state -> a = ans & 0xff;
			}
		case 0x81: //ADD C
			{

			   uint16_t ans = (uint16_t) state-> a + (uint16_t) state -> c;
			   state -> cc.z = ((ans & 0xff) == 0);
			   state -> cc.s = ((ans & 0x80) != 0);
			   state -> cc.cy = (ans > 0xff);
			   state -> cc.p = Parity(ans & 0xff);
			   state -> a = ans & 0xff;

			}
		case 0x82: 
			{
				uint16_t ans = (uint16_t) state -> a + (uint16_t) state -> d;
				state -> cc.z = ((ans & 0xff) == 0);
				state -> cc.s = ((ans & 0x80) != 0);
				state -> cc.cy = (ans > 0xff);
				state -> cc.p = Parity(ans & 0xff);
				state -> a = ans & 0xff;
			}
		case 0x83: //add e
			{
				uint16_t ans = (uint16_t) state -> a + (uint16_t) state -> e;
				state -> cc.z = ((ans & 0xff) == 0);
				state -> cc.s = ((ans & 0x80) != 0);
				state -> cc.cy = (ans > 0xff);
				state -> cc.p = Parity(ans & 0xff);
				state -> a = ans & 0xff;

			}
		case 0x84: //ADD H
			{
				uint16_t ans = (uint16_t) state -> a + (uint16_t) state -> h;
				state -> cc.z = ((ans & 0xff) == 0);
				state -> cc.s = ((ans & 0x80) != 0);
				state -> cc.cy = (ans > 0xff);
				state -> cc.p = Parity(ans & 0xff);
				state -> a = ans & 0xff;

			}
		case 0x85: //ADD L
			{
				uint16_t ans = (uint16_t) state -> a + (uint16_t) state -> l;
				state -> cc.z = ((ans & 0xff) == 0);
				state -> cc.s = ((ans & 0x80) != 0);
				state -> cc.cy = (ans > 0xff);
				state -> cc.p = Parity(ans & 0xff);
				state -> a = ans & 0xff;

			}
		case 0x86: //ADD M
			{
				uint16_t offset = (state->h << 8) | (state->l);
				uint16_t ans = (uint16_t) state->a + state->memory[offset];
				state -> cc.z = ((ans & 0xff) == 0);
				state -> cc.s = ((ans & 0x80) != 0);
				state -> cc.cy = (ans > 0xff);
				state -> cc.p = Parity(ans & 0xff);
				state -> a = ans & 0xff;
			}
		case 0x87: //ADD A
			{
				uint16_t ans = (uint16_t) state -> a + (uint16_t) state -> a;
				state -> cc.z = ((ans & 0xff) == 0);
				state -> cc.s = ((ans & 0x80) != 0);
				state -> cc.cy = (ans > 0xff);
				state -> cc.p = Parity(ans & 0xff);
				state -> a = ans & 0xff;

			}
		case 0x88: e8080_adc(state, &state -> a, state -> b, state -> cc.cy); break;
		case 0x89: e8080_adc(state, &state -> a, state -> c, state -> cc.cy); break;
		case 0x8a: e8080_adc(state, &state -> a, state -> d, state -> cc.cy); break;
		case 0x8b: e8080_adc(state, &state -> a, state -> e, state -> cc.cy); break;
		case 0x8c: e8080_adc(state, &state -> a, state -> h, state -> cc.cy); break;
		case 0x8d: e8080_adc(state, &state -> a, state -> l, state -> cc.cy); break;
		case 0x8e: //ADC M
			   {
				   uint16_t offset = (state -> h << 8) | (state -> l);
				   e8080_adc(state, &state -> a, state -> memory[offset], state -> cc.cy);
			   }
		case 0x8f: e8080_adc(state, &state -> a, state -> a,  state-> cc.cy); break;
                case 0x90: e8080_sub(state, &state -> a, state -> b); break;
                case 0x91: e8080_sub(state, &state -> a, state -> c); break;
                case 0x92: e8080_sub(state, &state -> a, state -> d); break;
                case 0x93: e8080_sub(state, &state -> a, state -> e); break;
                case 0x94: e8080_sub(state, &state -> a, state -> h); break;
                case 0x95: e8080_sub(state, &state -> a, state -> l); break;
                case 0x96: //SUB M
                           {
                                   uint16_t offset = (state -> h << 8 | (state -> l));
                                   e8080_sub(state, &state -> a, state -> memory[offset]);
                           }
                case 0x97: e8080_sub(state, &state -> a, state -> a); break;
                case 0x98: e8080_sbb(state, &state -> a, state -> b, state -> cc.cy); break; 
                case 0x99: e8080_sbb(state, &state -> a, state -> c, state -> cc.cy); break;
                case 0x9a: e8080_sbb(state, &state -> a, state -> d, state -> cc.cy); break;
                case 0x9b: e8080_sbb(state, &state -> a, state -> e, state -> cc.cy); break;
                case 0x9c: e8080_sbb(state, &state -> a, state -> h, state -> cc.cy); break;
                case 0x9d: e8080_sbb(state, &state -> a, state -> l, state -> cc.cy); break;
                case 0x9e: //SBB M
                           {
                                uint16_t offset = (state -> h << 8 | (state -> l));
                                e8080_sub(state, &state -> a, state -> memory[offset]);
                           }
                case 0x9f: e8080_sbb(state, &state -> a, state -> a, state -> cc.cy); break;
                case 0xa0: e8080_ana(state, &state -> b); break; //ANA B
                case 0xa1: e8080_ana(state, &state -> c); break; //ANA C
                case 0xa2: e8080_ana(state, &state -> d); break; //ANA D
                case 0xa3: e8080_ana(state, &state -> e); break; //ANA E
                case 0xa4: e8080_ana(state, &state -> h); break; //ANA H
                case 0xa5: e8080_ana(state, &state -> l); break; //ANA L
                case 0xa6: //ANA M 
                           {
                                   uint16_t offset = get_hl_pair(state);
                                   e8080_ana(state, &state->memory[offset]);
                           }
                case 0xa7: e8080_ana(state, &state -> a); break; //ANA A
        
                case 0xa8: e8080_xra(state, &state -> b); break; //XRA B
                case 0xa9: e8080_xra(state, &state -> c); break; //XRA C
                case 0xaa: e8080_xra(state, &state -> d); break; //XRA D
                case 0xab: e8080_xra(state, &state -> e); break; //XRA E
                case 0xac: e8080_xra(state, &state -> h); break; //XRA H
                case 0xad: e8080_xra(state, &state -> l); break; //XRA L
                case 0xae: //XRA M
                           {
                                   uint16_t offset = get_hl_pair(state);
                                   e8080_xra(state, &state -> memory[offset]);
                           }
                case 0xaf: e8080_xra(state, &state -> a); break; //XRA A
                case 0xc0: ncond_ret(state, &state -> cc.z); break; // RNZ
                case 0xc2: jmp_ncond(state, &state -> cc.z, opcode); break; //JNZ ADR
                case 0xc3:      //JMP ADR
                           state -> pc = (opcode[2] << 8) | opcode[1];
                           break;
                case 0xc4: ncond_call(state, &state -> cc.z); break; //CNZ adr
		case 0xc6: e8080_add(state, opcode[1], 0); break; //ADI byte	//MIGHT CAUSE ISSUES!!!
                case 0xc7: e8080_rst(state, get_hl_pair(state), 0); break; //RST 0
			   /*{
				uint16_t ans = (uint16_t) state -> a + (uint16_t) opcode[1];
			   	state -> cc.z = ((ans & 0xff) == 0);
			   	state -> cc.s = ((ans & 0x80) != 0);
			   	state -> cc.cy = (ans > 0xff);
			   	state -> cc.p = Parity(ans & 0xff);
			   	state -> a = ans & 0xff;
			}*/
                case 0xc8: cond_ret(state, &state -> cc.z); break;//RZ
                case 0xc9: e8080_ret(state); break; //RET
                case 0xca: jmp_cond(state, &state -> cc.z, opcode); break;//JZ ADR
                case 0xcc: cond_call(state, &state -> cc.z); break;//CZ ADR
                case 0xcd: e8080_call(state); break; //CALL ADR
                case 0xce: e8080_add(state, opcode[1], state -> cc.cy); break;//ACI D8 //MIGHT CAUSE ISSUES!
                case 0xcf: e8080_rst(state, get_hl_pair(state), 1); break; //RST 1
                case 0xd0: ncond_ret(state, &state -> cc.cy); break;//RNC
                case 0xd2: jmp_ncond(state, &state -> cc.cy, opcode); break;//JNC ADR
                                                                
                case 0xd4: ncond_call(state, &state -> cc.cy); break;//CNC ADR
                case 0xd7: e8080_rst(state, get_hl_pair(state), 2); break; //RST 2
                case 0xd8: cond_ret(state, &state -> cc.cy); break;//RC
                case 0xda: jmp_cond(state, &state -> cc.cy, opcode); break;//JC ADR
                case 0xdc: cond_call(state, &state -> cc.cy); break; //CC ADR
                case 0xde: e8080_sbb(state, &state -> a, opcode[1], &state -> cc.cy); break; //ISSUES???
                case 0xdf: e8080_rst(state, get_hl_pair(state); 3); break; //RST 3
                case 0xe0: ncond_ret(state, &state -> cc.p); break;//RPO
                case 0xe2: jmp_ncond(state, &state -> cc.p, opcode); break; //JPO ADR
                case 0xe4: ncond_call(state, &state -> cc.p); break; //CPO ADR
                case 0xe7: e8080_rst(state, get_hl_pair(state), 4); break; //RST 4
                case 0xe8: cond_ret(state, &state -> cc.p); break;//RPE
                case 0xe9: //PCHL
                           state -> pc = (state -> h << 8) | (state -> l);
                           break;
                case 0xea: jmp_cond(state, &state -> cc.p, opcode); break; //JPE ADR
                case 0xec: cond_call(state, &state -> cc.p); break; //CPE adr
                case 0xef: e8080_rst(state, get_hl_pair(state), 5); break; //RST 5
                case 0xf0: ncond_ret(state, &state -> cc.s); break; //RP
                case 0xf2: jmp_ncond(state, &state -> cc.s, opcode); break;//JP ADR
                case 0xf4: ncond_call(state, &state -> cc.s); break; //CP adr
                case 0xf7: e8080_rst(state, get_hl_pair(state), 6); break; //RST 6
                case 0xf8: cond_ret(state, &state -> cc.s); break;
                case 0xfa: jmp_cond(state, &state -> cc.s, opcode); break;//JM ADR
                case 0xfc: cond_call(state, &state -> cc.s); break; //CM adr
		case 0xfe: UnimplementedInstruction(state); break;
		case 0xff: e8080_rst(state, get_hl_pair(state), 7); break; //RST 7

	}
	state->pc +=1; //for opcode
}





/* 8080 Disassembling algorithm:
 * 1. Read code into buffer
 * 2. Get pointer to start of buffer
 * 3. Use byte at pointer to determine opcode
 * 4. print out the name of the opcode using the bytes ater the opcode as data, if applicable
 * 5. Advance the pointer the number of bytes used by that instruction
 * 6. If not at end of buffer, go to step 3
 */

/*
 * codebuffer is a valid pointer to 8080 assembly code
 * pc is the current offset into the code
 * returns the number of bytes of the op
 */

/*
 * Note to self: $ = hex, # = literal
 * So if we have #$%02x%02x, we want a hexadecimal literal where each byte is 2 characters long
 */

int Disassemble8080op(unsigned char *buffer, int pc){
	unsigned char *code = &buffer[pc];
	int opbytes = 1;
	printf("%04x ", pc);
	switch (*code){
		case 0x00: printf("NOP"); break;
		case 0x01: printf("LXI	B, #$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0x02: printf("STAX	B"); break;
		case 0x03: printf("INX	B"); break;
		case 0x04: printf("INR	B"); break;
		case 0x05: printf("DCR	B"); break;
		case 0x06: printf("MVI	B, #$%02x", code[1]); opbytes=2; break;
		case 0x07: printf("RLC"); break;
		case 0x08: printf("NOP"); break; //WHAT'S DEAL WITH EMPTY OPCODES?
		case 0x09: printf("DAD	B"); break;
		case 0x0a: printf("LDAX	B"); break;
		case 0x0b: printf("DCX	B"); break;
		case 0x0c: printf("INR	C"); break;
		case 0x0d: printf("DCR	C"); break;
		case 0x0e: printf("MVI	C, #$%02x", code[1]); opbytes=2; break;
		case 0x0f: printf("RRC"); break;

		case 0x10: printf("NOP"); break;
		case 0x11: printf("LXI	D, #$%02x%02x", code[2], code[1]); opbytes=3; break; //DBL CHECK THIS
		case 0x12: printf("STAX	D"); break;
		case 0x13: printf("INX	D"); break;
		case 0x14: printf("INR	D"); break;
		case 0x15: printf("DCR	D"); break;
		case 0x16: printf("MVI	D, #$%02x", code[1]); opbytes=2; break; //HERE
		case 0x17: printf("RAL"); break;
		case 0x18: printf("NOP"); break;
		case 0x19: printf("DAD	D"); break;
		case 0x1a: printf("LDAX	D"); break;
		case 0x1b: printf("DCX	D"); break;
		case 0x1c: printf("INR	E"); break;
		case 0x1d: printf("DCR	E"); break;
		case 0x1e: printf("MVI	E,	#$%02x", code[1]); opbytes=2; break;
		case 0x1f: printf("RAR"); break;

		case 0x20: printf("RIM"); break; //different from OG
		case 0x21: printf("LXI	H, #$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0x22: printf("SHLD $%02x%02x", code[2], code[1]); opbytes=3; break; //FIGURE THIS OUT
		case 0x23: printf("INX	H"); break;
		case 0x24: printf("INR	H"); break;
		case 0x25: printf("DCR	H"); break;
		case 0x26: printf("MVI	H, #$%02x", code[1]); opbytes=2; break;
		case 0x27: printf("DAA"); break;
		case 0x28: printf("NOP"); break;
		case 0x29: printf("DAD	H"); break;
		case 0x2a: printf("LHLD $%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0x2b: printf("DCX	H"); break;
		case 0x2c: printf("INR	L"); break;
		case 0x2d: printf("DCR	L"); break;
		case 0x2e: printf("MVI L, #$%02x", code[1]); opbytes=2; break;
		case 0x2f: printf("CMA"); break;

		case 0x30: printf("SIM"); break; //different from OG
		case 0x31: printf("LXI	SP, #$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0x32: printf("STA	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0x33: printf("INX	SP"); break;
		case 0x34: printf("INR	M"); break;
		case 0x35: printf("DCR	M"); break;
		case 0x36: printf("MVI 	M, #$%02x", code[1]); opbytes=2; break;
		case 0x37: printf("STC"); break;
		case 0x38: printf("NOP"); break;
		case 0x39: printf("DAD	SP"); break;
		case 0x3a: printf("LDA	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0x3b: printf("DCX	SP"); break;
		case 0x3c: printf("INR	A"); break;
		case 0x3d: printf("DCR	A"); break;
		case 0x3e: printf("MVI	A, #$%02x", code[1]); opbytes=2; break; //COME BACK TO THIS ONE ESP.!
		case 0x3f: printf("CMC"); break;

		case 0x40: printf("MOV	B, B"); break;
		case 0x41: printf("MOV	B, C"); break;
		case 0x42: printf("MOV	B, D"); break;
		case 0x43: printf("MOV	B, E"); break;
		case 0x44: printf("MOV	B, H"); break;
		case 0x45: printf("MOV	B, L"); break;
		case 0x46: printf("MOV	B, M"); break;
		case 0x47: printf("MOV	B, A"); break;
		case 0x48: printf("MOV	C, B"); break;
		case 0x49: printf("MOV	C, C"); break;
		case 0x4a: printf("MOV	C, D"); break;
		case 0x4b: printf("MOV	C, E"); break;
		case 0x4c: printf("MOV	C, H"); break;
		case 0x4d: printf("MOV	C, L"); break;
		case 0x4e: printf("MOV	C, M"); break;
		case 0x4f: printf("MOV	C, A"); break;

		case 0x50: printf("MOV 	D, B"); break;
		case 0x51: printf("MOV	D, C"); break;
		case 0x52: printf("MOV	D, D"); break;
		case 0x53: printf("MOV	D, E"); break;
		case 0x54: printf("MOV	D, H"); break;
		case 0x55: printf("MOV	D, L"); break;
		case 0x56: printf("MOV	D, M"); break;
		case 0x57: printf("MOV	D, A"); break;
		case 0x58: printf("MOV	E, B"); break;
		case 0x59: printf("MOV	E, C"); break;
		case 0x5a: printf("MOV	E, D"); break;
		case 0x5b: printf("MOV	E, E"); break;
		case 0x5c: printf("MOV	E, H"); break;
		case 0x5d: printf("MOV	E, L"); break;
		case 0x5e: printf("MOV	E, M"); break;
		case 0x5f: printf("MOV	E, A"); break;

		case 0x60: printf("MOV	H, B"); break;
		case 0x61: printf("MOV	H, C"); break;
		case 0x62: printf("MOV	H, D"); break;
		case 0x63: printf("MOV	H, E"); break;
		case 0x64: printf("MOV	H, H"); break;
		case 0x65: printf("MOV	H, L"); break;
		case 0x66: printf("MOV	H, M"); break;
		case 0x67: printf("MOV	H, A"); break;
		case 0x68: printf("MOV	L, B"); break;
		case 0x69: printf("MOV	L, C"); break;
		case 0x6a: printf("MOV	L, D"); break;
		case 0x6b: printf("MOV	L, E"); break;
		case 0x6c: printf("MOV	L, H"); break;
		case 0x6d: printf("MOV	L, L"); break;
		case 0x6e: printf("MOV	L, M"); break;
		case 0x6f: printf("MOV	L, A"); break;

		case 0x70: printf("MOV	M, B"); break;
		case 0x71: printf("MOV	M, C"); break;
		case 0x72: printf("MOV	M, D"); break;
		case 0x73: printf("MOV	M, E"); break;
		case 0x74: printf("MOV	M, H"); break;
		case 0x75: printf("MOV	M, L"); break;
		case 0x76: printf("HLT"); break;
		case 0x77: printf("MOV	M, A"); break;
		case 0x78: printf("MOV	A, B"); break;
		case 0x79: printf("MOV	A, C"); break;
		case 0x7a: printf("MOV	A, D"); break;
		case 0x7b: printf("MOV	A, E"); break;
		case 0x7c: printf("MOV	A, H"); break;
		case 0x7d: printf("MOV	A, L"); break;
		case 0x7e: printf("MOV	A, M"); break;
		case 0x7f: printf("MOV	A, A"); break;

		case 0x80: printf("ADD	B"); break;
		case 0x81: printf("ADD	C"); break;
		case 0x82: printf("ADD	D"); break;
		case 0x83: printf("ADD	E"); break;
		case 0x84: printf("ADD	H"); break;
		case 0x85: printf("ADD	L"); break;
		case 0x86: printf("ADD	M"); break;
		case 0x87: printf("ADD	A"); break;
		case 0x88: printf("ADC	B"); break;
		case 0x89: printf("ADC	C"); break;
		case 0x8a: printf("ADC	D"); break;
		case 0x8b: printf("ADC	E"); break;
		case 0x8c: printf("ADC	H"); break;
		case 0x8d: printf("ADC	L"); break;
		case 0x8e: printf("ADC	M"); break;
		case 0x8f: printf("ADC	A"); break;

		case 0x90: printf("SUB	B"); break;
		case 0x91: printf("SUB	C"); break;
		case 0x92: printf("SUB	D"); break;
		case 0x93: printf("SUB	E"); break;
		case 0x94: printf("SUB	H"); break;
		case 0x95: printf("SUB	L"); break;
		case 0x96: printf("SUB	M"); break;
		case 0x97: printf("SUB	A"); break;
		case 0x98: printf("SBB	B"); break;
		case 0x99: printf("SBB	C"); break;
		case 0x9a: printf("SBB	D"); break;
		case 0x9b: printf("SBB	E"); break;
		case 0x9c: printf("SBB	H"); break;
		case 0x9d: printf("SBB	L"); break;
		case 0x9e: printf("SBB	M"); break;
		case 0x9f: printf("SBB	A"); break;

		case 0xa0: printf("ANA	B"); break;
		case 0xa1: printf("ANA	C"); break;
		case 0xa2: printf("ANA	D"); break;
		case 0xa3: printf("ANA	E"); break;
		case 0xa4: printf("ANA	H"); break;
		case 0xa5: printf("ANA	L"); break;
		case 0xa6: printf("ANA	M"); break;
		case 0xa7: printf("ANA	A"); break;
		case 0xa8: printf("XRA	B"); break;
		case 0xa9: printf("XRA	C"); break;
		case 0xaa: printf("XRA	D"); break;
		case 0xab: printf("XRA	E"); break;
		case 0xac: printf("XRA	H"); break;
		case 0xad: printf("XRA	L"); break;
		case 0xae: printf("XRA	M"); break;
		case 0xaf: printf("XRA	A"); break;

		case 0xb0: printf("ORA	B"); break;
		case 0xb1: printf("ORA	C"); break;
		case 0xb2: printf("ORA	D"); break;
		case 0xb3: printf("ORA	E"); break;
		case 0xb4: printf("ORA	H"); break;
		case 0xb5: printf("ORA	L"); break;
		case 0xb6: printf("ORA	M"); break;
		case 0xb7: printf("ORA	A"); break;
		case 0xb8: printf("CMP	B"); break;
		case 0xb9: printf("CMP	C"); break;
		case 0xba: printf("CMP	D"); break;
		case 0xbb: printf("CMP	E"); break;
		case 0xbc: printf("CMP	H"); break;
		case 0xbd: printf("CMP	L"); break;
		case 0xbe: printf("CMP	M"); break;
		case 0xbf: printf("CMP	A"); break;

		case 0xc0: printf("RNZ"); break;
		case 0xc1: printf("POP	B"); break;
		case 0xc2: printf("JNZ	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xc3: printf("JMP	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xc4: printf("CNZ	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xc5: printf("PUSH	B"); break;
		case 0xc6: printf("ADI	#$%02x", code[1]); opbytes=2; break;
		case 0xc7: printf("RST	0"); break;
		case 0xc8: printf("RZ"); break;
		case 0xc9: printf("RET"); break;
		case 0xca: printf("JZ	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xcb: printf("NOP"); break; //different from OG
		case 0xcc: printf("CZ	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xcd: printf("CALL	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xce: printf("ACI	#$%02x", code[1]); opbytes=2; break;
		case 0xcf: printf("RST	1"); break;

		case 0xd0: printf("RNC"); break;
		case 0xd1: printf("POP	D"); break;
		case 0xd2: printf("JNC	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xd3: printf("OUT	#$%02x", code[1]); opbytes=2; break;
		case 0xd4: printf("CNC	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xd5: printf("PUSH	D"); break;
		case 0xd6: printf("SUI	#$%02x", code[1]); opbytes=2; break;
		case 0xd7: printf("RST	2"); break;
		case 0xd8: printf("RC"); break;
		case 0xd9: printf("NOP"); break;
		case 0xda: printf("JC	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xdb: printf("IN	#$%02x", code[1]); opbytes=2; break;
		case 0xdc: printf("CC	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xdd: printf("NOP"); break; //different from OG
		case 0xde: printf("SBI	#$%02x", code[1]); opbytes=2; break;
		case 0xdf: printf("RST	3"); break;

		case 0xe0: printf("RPO"); break;
		case 0xe1: printf("POP	H"); break;
		case 0xe2: printf("JPO	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xe3: printf("XTHL"); break;
		case 0xe4: printf("CPO	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xe5: printf("PUSH	H"); break;
		case 0xe6: printf("ANI	#$%02x", code[1]); opbytes=2; break;
		case 0xe7: printf("RST	4"); break;
		case 0xe8: printf("RPE"); break;
		case 0xe9: printf("PCHL"); break;
		case 0xea: printf("JPE	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xeb: printf("XCHG"); break;
		case 0xec: printf("CPE	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xed: printf("NOP"); break;
		case 0xee: printf("XRI #$%02x",code[1]); opbytes=2; break;
		case 0xef: printf("RST	5"); break; //WHY 5?

		case 0xf0: printf("RP"); break;
		case 0xf1: printf("POP	PSW"); break;
		case 0xf2: printf("JP	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xf3: printf("DI"); break;
		case 0xf4: printf("CP	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xf5: printf("PUSH	PSW"); break;
		case 0xf6: printf("ORI	#$%02x", code[1]); opbytes=2; break;
		case 0xf7: printf("RST	6"); break;
		case 0xf8: printf("RM"); break;
		case 0xf9: printf("SPHL"); break;
		case 0xfa: printf("JM	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xfb: printf("EI"); break;
		case 0xfc: printf("CM	$%02x%02x", code[2], code[1]); opbytes=3; break;
		case 0xfd: printf("NOP"); break; //WHATS DEAL WITH THIS INSTRUCTION VS REPO??
		case 0xfe: printf("CPI	#$%02x", code[1]); opbytes=2; break;
		case 0xff: printf("RST	7"); break;
	}
	return opbytes;
}

int main(int argc, char**argv){
	FILE *f = fopen(argv[1], "rb");
	if (f==NULL){
		printf("Error: Could not open %s\n", argv[1]);
		exit(1);
	}
	//get file size and read it into memory buffer
	fseek(f, 0L, SEEK_END);
	int fsize = ftell(f);
	fseek(f, 0L, SEEK_SET);
        
        unsigned char *buffer = malloc(fsize);

	fread(buffer, fsize, 1, f);
	fclose(f);

	int prog_counter = 0;

	while(prog_counter < fsize){
		int advance = Disassemble8080op(buffer, prog_counter);
		prog_counter += advance;
		printf("\n");
	}
	return 0;
}

