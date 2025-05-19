#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Bytecode opcodes
typedef enum {
    OP_PUSHN = 0,        // Push a number onto the stack
    OP_ADD = 1,          // Add two numbers
    OP_SUB = 2,          // Subtract
    OP_MUL = 3,          // Multiply
    OP_DIV = 4,          // Divide
    OP_MOD = 5,          // Modulo
    OP_SIGN = 6,         // Sign of a number
    OP_ALLOCATE = 7,     // Allocate memory on heap
    OP_FREE = 8,         // Free memory on heap
    OP_BEGIN_WHILE = 9,  // Begin a while loop
    OP_END_WHILE = 10,   // End a while loop
    OP_STORE = 11,       // Store bytes in memory
    OP_LOAD = 12,        // Load bytes from memory
    OP_CALL = 13,        // Call a function
    OP_LOAD_FRAME_PTR = 14, // Load the frame pointer
    OP_MAKE_STACK_FRAME = 15, // Create a stack frame
    OP_DROP_STACK_FRAME = 16, // Drop a stack frame
    OP_POPSTR = 17,      // Output memory contents to stdout
    OP_DUP = 18,         // Duplicate the top of stack
    OP_BREAKPT = 19      // Breakpoint/print the stack
} Opcode;

void write_byte(FILE* file, uint8_t byte) {
    // Write byte as two hex characters
    fprintf(file, "%02x ", byte);
}

void write_i16(FILE* file, int16_t value) {
    // Write in little-endian format as hex
    write_byte(file, value & 0xFF);
    write_byte(file, (value >> 8) & 0xFF);
    fprintf(file, "\n");
}

void write_u24(FILE* file, uint32_t value) {
    // Write in little-endian format (24-bit) as hex
    write_byte(file, value & 0xFF);
    write_byte(file, (value >> 8) & 0xFF);
    write_byte(file, (value >> 16) & 0xFF);
    fprintf(file, "\n");
}

int main() {
    FILE* file = fopen("store_test.ppx", "w");
    if (!file) {
        perror("Failed to create output file");
        exit(1);
    }

    fprintf(file, "# Store Test\n");
    fprintf(file, "# This program stores values at 3 different addresses and then verifies them\n");
    fprintf(file, "# by dumping the memory contents with OP_POPSTR\n\n");

    // Address 1: Store at heap address 100 the value 65 (ASCII 'A')
    write_byte(file, OP_PUSHN);    // Push address 100
    write_i16(file, 100);

    write_byte(file, OP_PUSHN);    // Push value 65 ('A')
    write_i16(file, 65);

    write_byte(file, OP_STORE);    // Store 1 byte
    write_u24(file, 1);

    // Address 2: Store at heap address 200 the value 66 (ASCII 'B')
    write_byte(file, OP_PUSHN);    // Push address 200
    write_i16(file, 101);

    write_byte(file, OP_PUSHN);    // Push value 66 ('B')
    write_i16(file, 66);

    write_byte(file, OP_STORE);    // Store 1 byte
    write_u24(file, 1);

    // Address 3: Store at heap address 300 the value 67 (ASCII 'C')
    write_byte(file, OP_PUSHN);    // Push address 300
    write_i16(file, 102);

    write_byte(file, OP_PUSHN);    // Push value 67 ('C')
    write_i16(file, 67);

    write_byte(file, OP_STORE);    // Store 1 byte
    write_u24(file, 1);

    // Print debug stack after storing
    write_byte(file, OP_BREAKPT);

    // Dump memory at address 100 (should show 'A')
    write_byte(file, OP_PUSHN);    // Push address 100
    write_i16(file, 100);

    write_byte(file, OP_PUSHN);    // Push length 1
    write_i16(file, 1);

    write_byte(file, OP_POPSTR);   // Dump memory

    // Dump memory at address 200 (should show 'B')
    write_byte(file, OP_PUSHN);    // Push address 200
    write_i16(file, 101);

    write_byte(file, OP_PUSHN);    // Push length 1
    write_i16(file, 1);

    write_byte(file, OP_POPSTR);   // Dump memory

    // Dump memory at address 300 (should show 'C')
    write_byte(file, OP_PUSHN);    // Push address 300
    write_i16(file, 102);

    write_byte(file, OP_PUSHN);    // Push length 1
    write_i16(file, 1);

    write_byte(file, OP_POPSTR);   // Dump memory

    // Now let's dump all three bytes in one go
    write_byte(file, OP_PUSHN);    // Push address 100
    write_i16(file, 100);

    write_byte(file, OP_PUSHN);
    write_i16(file, 3);

    write_byte(file, OP_POPSTR);   // Dump all memory from 100 to 102

    fclose(file);
    printf("Generated store_test.ppx\n");
    return 0;
}
