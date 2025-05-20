// som_parser.h - SOM language parser for Poplar2

#ifndef POPLAR2_SOM_PARSER_H
#define POPLAR2_SOM_PARSER_H

#include "vm.h"
#include <stdbool.h>
#include <stdio.h>

// Token types
typedef enum {
    TOKEN_EOF,           // End of file
    TOKEN_IDENTIFIER,    // Identifier
    TOKEN_KEYWORD,       // Keyword (ending with ':')
    TOKEN_INTEGER,       // Integer literal
    TOKEN_COMMENT,       // comment literal \".*\"
    TOKEN_STRING,        // String literal
    TOKEN_SYMBOL,        // Symbol literal (#symbol)
    TOKEN_OPERATOR,      // Operator (+, -, *, /, etc.)
    TOKEN_SEPARATOR,     // Special character (., |, etc.)
    TOKEN_LPAREN,        // (
    TOKEN_RPAREN,        // )
    TOKEN_LBRACE,        // {
    TOKEN_RBRACE,        // }
    TOKEN_LBRACKET,      // [
    TOKEN_RBRACKET,      // ]
    TOKEN_CARET,         // ^
    TOKEN_COLON,         // :
    TOKEN_ASSIGN,        // :=
    TOKEN_PERIOD,        // .
    TOKEN_PRIMITIVE,     // <primitive: ...>
    TOKEN_ERROR          // Error token
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char* text;
    int length;
    int line;
    int column;
} Token;

// Lexer structure
typedef struct {
    const char* source;
    const char* filename;
    const char* start;
    const char* current;
    int line;
    int column;
    Token token;
    Token previous;
    bool had_error;
    bool panic_mode;
} Lexer;

// Parser structure
typedef struct {
    Lexer lexer;
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
    int class_index;          // Current global index for classes
} Parser;

// Token handling functions (internal, not exposed)

// Parser initialization and file handling
bool parse_file(const char* filename);
bool parse_string(const char* source, const char* name);

// Class and method parsing
Value parse_class(Parser* parser);

// Access to parser error state
bool parser_had_error();
void parser_reset_error();

// For debug purposes
void print_tokens(const char* source);

#endif /* POPLAR2_SOM_PARSER_H */
