/* 8080 Disassembling algorithm:
 * 1. Read code into buffer
 * 2. Get pointer to start of buffer
 * 3. Use byte at pointer to determine opcode
 * 4. print out the name of the opcode using the bytes ater the opcode as data, if applicable
 * 5. Advance the pointer the number of bytes used by that instruction
 * 6. If not at end of buffer, go to step 3
 */
