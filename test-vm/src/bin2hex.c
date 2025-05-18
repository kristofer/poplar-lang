#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Function prototypes
void convert_file(const char* input_file, const char* output_file);
void print_usage(const char* program_name);

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        print_usage(argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];
    
    convert_file(input_file, output_file);
    
    return 0;
}

void convert_file(const char* input_file, const char* output_file) {
    FILE* in_fp = fopen(input_file, "rb");
    if (!in_fp) {
        fprintf(stderr, "Error: Could not open input file %s\n", input_file);
        exit(1);
    }
    
    FILE* out_fp = fopen(output_file, "w");
    if (!out_fp) {
        fprintf(stderr, "Error: Could not open output file %s\n", output_file);
        fclose(in_fp);
        exit(1);
    }
    
    // Write header comment
    fprintf(out_fp, "# Poplar bytecode file converted from binary to hex ASCII format\n");
    fprintf(out_fp, "# Original file: %s\n", input_file);
    fprintf(out_fp, "# Format: Each byte is represented by two hex characters\n");
    fprintf(out_fp, "# Line breaks and comments are ignored by the VM\n\n");
    
    unsigned char buffer[16];
    size_t bytes_read;
    size_t total_bytes = 0;
    int line_pos = 0;
    
    // Read file in chunks and convert to hex
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), in_fp)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            fprintf(out_fp, "%02x", buffer[i]);
            line_pos++;
            
            // Add space every 2 bytes for readability
            if (line_pos % 2 == 0) {
                fprintf(out_fp, " ");
            }
            
            // Add line break every 16 bytes for readability
            if (line_pos % 16 == 0) {
                fprintf(out_fp, "\n");
            }
            
            total_bytes++;
        }
    }
    
    // Add final newline if needed
    if (line_pos % 16 != 0) {
        fprintf(out_fp, "\n");
    }
    
    // Write footer with statistics
    fprintf(out_fp, "\n# Total bytes converted: %zu\n", total_bytes);
    
    fclose(in_fp);
    fclose(out_fp);
    
    printf("Converted %zu bytes from %s to %s in hex ASCII format\n", 
           total_bytes, input_file, output_file);
}

void print_usage(const char* program_name) {
    printf("Poplar Binary to Hex ASCII Converter\n");
    printf("Usage: %s <input_binary_file> <output_hex_file>\n", program_name);
    printf("Converts a binary .ppx file to a human-readable hex ASCII format\n");
}