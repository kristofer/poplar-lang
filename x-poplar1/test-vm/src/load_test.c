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
    FILE* file = fopen("load_test.ppx", "w");
    if (!file) {
        perror("Failed to create output file");
        exit(1);
    }

    fprintf(file, "# Load Test\n");
    fprintf(file, "# This program tests the OP_LOAD operation\n");
    fprintf(file, "# It stores values at memory addresses and then loads them back\n");
    fprintf(file, "# Note: vm_load pushes bytes in reverse order - last byte first\n\n");

    // First initialize three memory locations with test values
    // Address 100 = 'A' (65)
    write_byte(file, OP_PUSHN);    // Push value 'A'
    write_i16(file, 65);
    write_byte(file, OP_PUSHN);    // Push address 100
    write_i16(file, 100);
    write_byte(file, OP_STORE);    // Store 1 byte
    write_u24(file, 1);

    // Address 101 = 'B' (66)
    write_byte(file, OP_PUSHN);    // Push value 'B'
    write_i16(file, 66);
    write_byte(file, OP_PUSHN);    // Push address 101
    write_i16(file, 101);
    write_byte(file, OP_STORE);    // Store 1 byte
    write_u24(file, 1);

    // Address 102 = 'C' (67)
    write_byte(file, OP_PUSHN);    // Push value 'C'
    write_i16(file, 67);
    write_byte(file, OP_PUSHN);    // Push address 102
    write_i16(file, 102);
    write_byte(file, OP_STORE);    // Store 1 byte
    write_u24(file, 1);

    // Show memory contents for verification
    write_byte(file, OP_PUSHN);    // Push address 100
    write_i16(file, 100);
    write_byte(file, OP_PUSHN);    // Push length 3
    write_i16(file, 3);
    write_byte(file, OP_POPSTR);   // Display memory

    // Clear stack before testing LOAD operations
    write_byte(file, OP_BREAKPT);  // Show stack state

    // Test 1: Load a single byte from address 100 (should be 'A')
    write_byte(file, OP_PUSHN);    // Push address 100
    write_i16(file, 100);
    write_byte(file, OP_LOAD);     // Load 1 byte onto stack
    write_u24(file, 1);
    write_byte(file, OP_BREAKPT);  // Show stack state with loaded value - should have 'A' (65)

    // Test 2: Load a single byte from address 101 (should be 'B')
    write_byte(file, OP_PUSHN);    // Push address 101
    write_i16(file, 101);
    write_byte(file, OP_LOAD);     // Load 1 byte onto stack
    write_u24(file, 1);
    write_byte(file, OP_BREAKPT);  // Show stack state with loaded value - should have 'B' (66)

    // Test 3: Load a single byte from address 102 (should be 'C')
    write_byte(file, OP_PUSHN);    // Push address 102
    write_i16(file, 102);
    write_byte(file, OP_LOAD);     // Load 1 byte onto stack
    write_u24(file, 1);
    write_byte(file, OP_BREAKPT);  // Show stack state with loaded value - should have 'C' (67)

    // Test 4: Load multiple bytes at once from address 100 (should be 'A', 'B', 'C')
    write_byte(file, OP_PUSHN);    // Push address 100
    write_i16(file, 100);
    write_byte(file, OP_LOAD);     // Load 3 bytes onto stack
    write_u24(file, 3);
    write_byte(file, OP_BREAKPT);  // Show stack with loaded values (67 'C', 66 'B', 65 'A')

    // Final test: Store loaded values back into different memory locations
    // Values on stack should be (top to bottom): C, B, A
    // Store 'C' (67) at address 200
    write_byte(file, OP_PUSHN);    // Push address 200
    write_i16(file, 200);
    write_byte(file, OP_STORE);    // Store 1 byte
    write_u24(file, 1);

    // Store 'B' (66) at address 201
    write_byte(file, OP_PUSHN);    // Push address 201
    write_i16(file, 201);
    write_byte(file, OP_STORE);    // Store 1 byte
    write_u24(file, 1);

    // Store 'A' (65) at address 202
    write_byte(file, OP_PUSHN);    // Push address 202
    write_i16(file, 202);
    write_byte(file, OP_STORE);    // Store 1 byte
    write_u24(file, 1);

    // Verify the copied values
    write_byte(file, OP_PUSHN);    // Push address 200
    write_i16(file, 200);
    write_byte(file, OP_PUSHN);    // Push length 3
    write_i16(file, 3);
    write_byte(file, OP_POPSTR);   // Display memory

    fclose(file);
    printf("Generated load_test.ppx\n");
    return 0;
}
