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
    FILE* file = fopen("simple_string_test.ppx", "w");
    if (!file) {
        perror("Failed to create output file");
        exit(1);
    }

    fprintf(file, "# Simple String Test\n");
    fprintf(file, "# This program stores and prints a simple string using OP_POPSTR\n");
    fprintf(file, "# with the corrected vm_store behavior\n\n");

    const char* message = "Hello, world!";
    int length = strlen(message);
    int memory_addr = 100;  // Store at address 100

    // Allocate memory
    write_byte(file, OP_PUSHN);    // Push size
    write_i16(file, length);
    write_byte(file, OP_ALLOCATE);  // Allocate memory

    // Save the memory address for later
    write_byte(file, OP_DUP);       // Duplicate the address

    // Push the characters onto the stack in reverse order
    // (last character first, since vm_store pops them in reverse)
    for (int i = 0; i < length; i++) {
        write_byte(file, OP_PUSHN);
        write_i16(file, message[i]);
    }

    // Store the characters in memory
    write_byte(file, OP_STORE);
    write_u24(file, length);

    // Print debug information
    write_byte(file, OP_BREAKPT);

    // Print the string (address is still on stack from DUP earlier)
    write_byte(file, OP_PUSHN);    // Push length
    write_i16(file, length);
    write_byte(file, OP_POPSTR);   // Print the string

    fclose(file);
    printf("Generated simple_string_test.ppx\n");
    return 0;
}