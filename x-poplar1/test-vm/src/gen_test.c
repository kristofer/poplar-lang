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
    OP_END_WHILE = 10,    // End a while loop
    OP_STORE = 11,        // Store bytes in memory
    OP_LOAD = 12,         // Load bytes from memory
    OP_CALL = 13,         // Call a function
    OP_LOAD_FRAME_PTR = 14, // Load the frame pointer
    OP_MAKE_STACK_FRAME = 15, // Create a stack frame
    OP_DROP_STACK_FRAME = 16,  //x0F Drop a stack frame
    OP_POPSTR = 17, //x11 Output memory contents to stdout
    OP_DUP = 18, //x12 // dupe the TOS
    OP_BREAKPT = 19 //x13 // print the stack
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

// Generate a simple test program: Calculate and print 1+2*3
void generate_simple_math() {
    FILE* file = fopen("simple_math.ppx", "w");
    if (!file) {
        perror("Failed to create output file");
        exit(1);
    }

    // Push 1
    write_byte(file, OP_PUSHN);
    write_i16(file, 1);

    // Push 2
    write_byte(file, OP_PUSHN);
    write_i16(file, 2);

    // Push 3
    write_byte(file, OP_PUSHN);
    write_i16(file, 3);

    // Multiply 2*3
    write_byte(file, OP_MUL);

    // Add 1+(2*3)
    write_byte(file, OP_ADD);

    fclose(file);
    printf("Generated simple_math.ppx\n");
}

// Generate a test program: Allocate memory, store a string, then print it
void generate_hello_world() {
    FILE* file = fopen("hello_world.ppx", "w");
    if (!file) {
        perror("Failed to create output file");
        exit(1);
    }

    const char* message = "Hello, World!";
    int length = 13;  // Length of "Hello, World!"

    // Allocate memory for the string
    write_byte(file, OP_PUSHN);
    write_i16(file, length);
    write_byte(file, OP_ALLOCATE);
    write_byte(file, OP_DUP); // for the print below;

    // Push each character onto the stack in reverse order
    for (int i = length - 1; i >= 0; i--) {
        write_byte(file, OP_PUSHN);
        write_i16(file, message[i]);
    }

    // Store the characters in memory
    write_byte(file, OP_STORE);
    write_u24(file, length);

    // Output the string to stdout (push the pointer and length)
    // the pointer should be duped, and now at TOS;

    write_byte(file, OP_PUSHN);    // Push the length of the string
    write_i16(file, length);

    // Output the string
    write_byte(file, OP_POPSTR);

    fclose(file);
    printf("Generated hello_world.ppx\n");
}

// Generate a test program: Loop from 10 down to 1
void generate_countdown() {
    FILE* file = fopen("countdown.ppx", "w");
    if (!file) {
        perror("Failed to create output file");
        exit(1);
    }

    // Initialize counter to 10
    write_byte(file, OP_PUSHN);
    write_i16(file, 10);

    // Begin while loop (check if counter > 0)
    write_byte(file, OP_PUSHN);   // Duplicate counter for comparison
    write_i16(file, 0);
    write_byte(file, OP_SUB);     // counter - 0
    write_byte(file, OP_SIGN);    // Sign(counter - 0) => 1 if counter > 0
    write_byte(file, OP_BEGIN_WHILE);

    // Decrement counter
    write_byte(file, OP_PUSHN);
    write_i16(file, 1);
    write_byte(file, OP_SUB);

    // Check if counter > 0 for next iteration
    write_byte(file, OP_PUSHN);   // Duplicate counter for comparison
    write_i16(file, 0);
    write_byte(file, OP_SUB);     // counter - 0
    write_byte(file, OP_SIGN);    // Sign(counter - 0) => 1 if counter > 0

    // End while loop
    write_byte(file, OP_END_WHILE);

    fclose(file);
    printf("Generated countdown.ppx\n");
}

// Utility function to add comments to hex files
void add_file_comments() {
    // Add comments to simple_math.ppx
    FILE* file = fopen("simple_math.ppx", "r");
    if (!file) return;

    char buffer[1024];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    file = fopen("simple_math.ppx", "w");
    if (!file) return;

    fprintf(file, "# Simple math program: Calculate 1+2*3\n");
    fprintf(file, "# Format: Each byte is represented by two hex characters\n");
    fprintf(file, "# Opcodes: 00=PUSHN, 03=MUL, 01=ADD\n");
    fprintf(file, "# Line breaks and comments are ignored by the VM\n\n");
    fprintf(file, "# PUSHN 1\n00 0100\n\n");
    fprintf(file, "# PUSHN 2\n00 0200\n\n");
    fprintf(file, "# PUSHN 3\n00 0300\n\n");
    fprintf(file, "# MUL (2*3)\n03\n\n");
    fprintf(file, "# ADD (1+(2*3))\n01\n");
    fclose(file);

    // Add comments to hello_world.ppx
    file = fopen("hello_world.ppx", "r+");
    if (!file) return;

    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    file = fopen("hello_world.ppx", "w");
    if (!file) return;

    fprintf(file, "# Hello World program\n");
    fprintf(file, "# Format: Each byte is represented by two hex characters\n");
    fprintf(file, "# This program allocates memory, stores \"Hello, World!\", and outputs it to stdout\n\n");
    fprintf(file, "%s", buffer);
    fclose(file);

    // Add comments to countdown.ppx
    file = fopen("countdown.ppx", "r+");
    if (!file) return;

    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    file = fopen("countdown.ppx", "w");
    if (!file) return;

    fprintf(file, "# Countdown program: Loop from 10 down to 1\n");
    fprintf(file, "# Format: Each byte is represented by two hex characters\n");
    fprintf(file, "# This program demonstrates while loop functionality\n\n");
    fprintf(file, "%s", buffer);
    fclose(file);
}

// Generate a test program: Demonstrate memory output functionality
void generate_memory_dump() {
    FILE* file = fopen("memory_dump.ppx", "w");
    if (!file) {
        perror("Failed to create output file");
        exit(1);
    }

    const char* message = "Memory dump test string!";
    int length = strlen(message);  // Length of the string

    // Allocate memory for the string
    write_byte(file, OP_PUSHN);
    write_i16(file, length);
    write_byte(file, OP_ALLOCATE);
    write_byte(file, OP_DUP);  // Duplicate the pointer for later use

    // Push each character onto the stack in reverse order
    for (int i = length - 1; i >= 0; i--) {
        write_byte(file, OP_PUSHN);
        write_i16(file, message[i]);
    }

    // Store the characters in memory
    write_byte(file, OP_STORE);
    write_u24(file, length);

    // The pointer is already on the stack (from the DUP)

    // Push the number of bytes to output
    write_byte(file, OP_PUSHN);
    write_i16(file, length);

    // Output the memory contents
    write_byte(file, OP_POPSTR);

    fclose(file);
    printf("Generated memory_dump.ppx\n");
}

// Generate a very simple test program to prove OP_POPSTR works
void generate_simple_print() {
    FILE* file = fopen("simple_print.ppx", "w");
    if (!file) {
        perror("Failed to create output file");
        exit(1);
    }

    const char* message = "Simple print test!";
    int length = strlen(message);

    // Push each byte of the string directly into memory location 100-117
    for (int i = 0; i < length; i++) {
        // Push the memory address (100 + i)
        write_byte(file, OP_PUSHN);
        write_i16(file, 100 + i);
        
        // Push the character
        write_byte(file, OP_PUSHN);
        write_i16(file, message[i]);
        
        // Store just one byte
        write_byte(file, OP_STORE);
        write_u24(file, 1);
    }
    
    // Push the base address (100)
    write_byte(file, OP_PUSHN);
    write_i16(file, 100);
    
    // Push the length
    write_byte(file, OP_PUSHN);
    write_i16(file, length);
    
    // Print debug
    write_byte(file, OP_BREAKPT);
    
    // Output the string
    write_byte(file, OP_POPSTR);

    fclose(file);
    printf("Generated simple_print.ppx\n");
}

// Add comments to simple_print.ppx
void add_simple_print_comments() {
    FILE* file = fopen("simple_print.ppx", "r+");
    if (!file) return;

    char buffer[1024];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    file = fopen("simple_print.ppx", "w");
    if (!file) return;

    fprintf(file, "# Simple Print Test\n");
    fprintf(file, "# This is the simplest possible test of OP_POPSTR\n");
    fprintf(file, "# It writes directly to fixed memory locations (100-117)\n");
    fprintf(file, "# and then uses OP_POPSTR to print it\n\n");
    fprintf(file, "%s", buffer);
    fclose(file);
}

// Add comments to memory_dump.ppx
void add_memory_dump_comments() {
    FILE* file = fopen("memory_dump.ppx", "r+");
    if (!file) return;

    char buffer[1024];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    file = fopen("memory_dump.ppx", "w");
    if (!file) return;

    fprintf(file, "# Memory Dump program\n");
    fprintf(file, "# Format: Each byte is represented by two hex characters\n");
    fprintf(file, "# This program demonstrates the OP_POPSTR functionality\n");
    fprintf(file, "# by storing a text string in memory and then outputting it\n\n");
    fprintf(file, "%s", buffer);
    fclose(file);
}


int main() {
    generate_simple_math();
    generate_hello_world();
    generate_countdown();
    generate_memory_dump();
    generate_simple_print();

    // Add comments to make files more readable
    //add_file_comments();
    add_memory_dump_comments();
    add_simple_print_comments();

    printf("All test files generated successfully in ASCII hex format.\n");
    return 0;
}
