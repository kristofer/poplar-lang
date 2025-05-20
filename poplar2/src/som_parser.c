// som_parser.c - SOM language parser implementation for Poplar2

#include "som_parser.h"
#include "vm.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Error reporting
static void error(Parser* parser, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;

    fprintf(stderr, "[%s:%d:%d] Error: %s\n",
            parser->lexer.filename,
            parser->lexer.line,
            parser->lexer.column,
            message);

    parser->had_error = true;
    parser->lexer.had_error = true;
}

static void error_at_current(Parser* parser, const char* message) {
    error(parser, message);
}

static void error_at_previous(Parser* parser, const char* message) {
    parser->panic_mode = true;

    fprintf(stderr, "[%s:%d:%d] Error at '%.*s': %s\n",
            parser->lexer.filename,
            parser->previous.line,
            parser->previous.column,
            parser->previous.length,
            parser->previous.text,
            message);

    parser->had_error = true;
    parser->lexer.had_error = true;
}

// Lexer functions
static bool is_at_end(Lexer* lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer* lexer) {
    lexer->current++;
    lexer->column++;
    return lexer->current[-1];
}

static char peek(Lexer* lexer) {
    return *lexer->current;
}

static char peek_next(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static bool lexer_match(Lexer* lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->current != expected) return false;

    lexer->current++;
    lexer->column++;
    return true;
}

static void skip_whitespace(Lexer* lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '\n':
                lexer->line++;
                lexer->column = 0;
                advance(lexer);
                break;
            case '"':
                // This is a comment - consume the opening quote
                advance(lexer);

                // Continue until we find the closing quote or reach EOF
                while (!is_at_end(lexer)) {
                    if (peek(lexer) == '"') {
                        advance(lexer); // Consume the closing quote
                        break;     // Exit the comment-consuming loop
                    }

                    if (peek(lexer) == '\n') {
                        lexer->line++;
                        lexer->column = 0;
                    }

                    advance(lexer);
                }
                break;
            default:
                return;
        }
    }
}


static Token make_token(Lexer* lexer, TokenType type) {
    Token token;
    token.type = type;
    token.text = (char*)lexer->start;
    token.length = (int)(lexer->current - lexer->start);
    token.line = lexer->line;
    token.column = lexer->column - token.length;
    return token;
}

static Token error_token(Lexer* lexer, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.text = (char*)message;
    token.length = (int)strlen(message);
    token.line = lexer->line;
    token.column = lexer->column;
    return token;
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_identifier_start(char c) {
    return is_alpha(c);
}

static bool is_identifier_part(char c) {
    return is_alpha(c) || is_digit(c);
}

static Token identifier(Lexer* lexer) {
    while (is_identifier_part(peek(lexer))) {
        advance(lexer);
    }

    // Check if we have a keyword (ends with ':')
    if (peek(lexer) == ':') {
        advance(lexer);
        return make_token(lexer, TOKEN_KEYWORD);
    }

    return make_token(lexer, TOKEN_IDENTIFIER);
}

static Token number(Lexer* lexer) {
    while (is_digit(peek(lexer))) {
        advance(lexer);
    }

    return make_token(lexer, TOKEN_INTEGER);
}

static Token string(Lexer* lexer) {
    // Skip the opening quote
    advance(lexer);

    lexer->start = lexer->current;

    while (peek(lexer) != '\'' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') {
            return error_token(lexer, "Unterminated string");
        }
        advance(lexer);
    }

    // Make token but don't include the quotes
    Token token = make_token(lexer, TOKEN_STRING);

    // Skip the closing quote
    if (!is_at_end(lexer)) {
        advance(lexer);
    }

    return token;
}

static Token symbol(Lexer* lexer) {
    // Skip the #
    advance(lexer);

    // Read the symbol name
    if (is_identifier_start(peek(lexer))) {
        lexer->start = lexer->current;
        return identifier(lexer);
    } else if (peek(lexer) == '\'') {
        // Symbol with string syntax
        advance(lexer);
        lexer->start = lexer->current;

        while (peek(lexer) != '\'' && !is_at_end(lexer)) {
            advance(lexer);
        }

        Token token = make_token(lexer, TOKEN_SYMBOL);
        if (!is_at_end(lexer)) {
            advance(lexer); // Skip closing quote
        }
        return token;
    }

    return error_token(lexer, "Unexpected character after #");
}

static Token primitive(Lexer* lexer) {
    // Skip the opening '<'
    advance(lexer);

    // Check for "primitive:"
    const char* primitive = "primitive:";
    for (int i = 0; primitive[i] != '\0'; i++) {
        if (peek(lexer) != primitive[i]) {
            return error_token(lexer, "Expected 'primitive:' after '<'");
        }
        advance(lexer);
    }

    // Skip whitespace
    while (peek(lexer) == ' ' || peek(lexer) == '\t') {
        advance(lexer);
    }

    // Read the primitive number
    lexer->start = lexer->current;
    while (is_digit(peek(lexer))) {
        advance(lexer);
    }

    // Skip trailing whitespace
    while (peek(lexer) == ' ' || peek(lexer) == '\t') {
        advance(lexer);
    }

    // Check for closing '>'
    if (peek(lexer) != '>') {
        return error_token(lexer, "Expected '>' after primitive number");
    }
    advance(lexer);

    return make_token(lexer, TOKEN_PRIMITIVE);
}

static Token operator_token(Lexer* lexer) {
    while (strchr("+-*/=<>%&|~,@", peek(lexer)) != NULL) {
        advance(lexer);
    }

    return make_token(lexer, TOKEN_OPERATOR);
}

static Token scan_token(Lexer* lexer) {
    skip_whitespace(lexer);

    lexer->start = lexer->current;

    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }

    char c = advance(lexer);

    if (is_identifier_start(c)) {
        return identifier(lexer);
    }

    if (is_digit(c)) {
        return number(lexer);
    }

    switch (c) {
        case '(': return make_token(lexer, TOKEN_LPAREN);
        case ')': return make_token(lexer, TOKEN_RPAREN);
        case '{': return make_token(lexer, TOKEN_LBRACE);
        case '}': return make_token(lexer, TOKEN_RBRACE);
        case '[': return make_token(lexer, TOKEN_LBRACKET);
        case ']': return make_token(lexer, TOKEN_RBRACKET);
        case '^': return make_token(lexer, TOKEN_CARET);
        case '.': return make_token(lexer, TOKEN_PERIOD);
        case '|': return make_token(lexer, TOKEN_SEPARATOR);
        case ':':
            if (lexer_match(lexer, '=')) {
                return make_token(lexer, TOKEN_ASSIGN);
            }
            return make_token(lexer, TOKEN_COLON);
        case '\'': return string(lexer);
        case '#': return symbol(lexer);
        case '<':
            if (peek(lexer) == 'p') {
                return primitive(lexer);
            }
            return operator_token(lexer);
        case '+': case '-': case '*': case '/':
        case '=': case '>': case '%': case '&':
        case '~': case ',': case '@':
            return operator_token(lexer);
    }

    return error_token(lexer, "Unexpected character");
}

// Function declarations
static void error(Parser* parser, const char* message);
static void error_at_current(Parser* parser, const char* message);
static void error_at_previous(Parser* parser, const char* message);
static bool is_at_end(Lexer* lexer);
static char advance(Lexer* lexer);
static char peek(Lexer* lexer);
static char peek_next(Lexer* lexer);
static bool lexer_match(Lexer* lexer, char expected);
static void skip_whitespace(Lexer* lexer);
static Token make_token(Lexer* lexer, TokenType type);
static Token error_token(Lexer* lexer, const char* message);
static bool is_alpha(char c);
static bool is_digit(char c);
static bool is_identifier_start(char c);
static bool is_identifier_part(char c);
static Token identifier(Lexer* lexer);
static Token number(Lexer* lexer);
static Token string(Lexer* lexer);
static Token symbol(Lexer* lexer);
static Token primitive(Lexer* lexer);
static Token operator_token(Lexer* lexer);
static Token scan_token(Lexer* lexer);
static void init_parser(Parser* parser, const char* source, const char* filename);
static void advance_token(Parser* parser);
static void consume(Parser* parser, TokenType type, const char* message);
static bool check(Parser* parser, TokenType type);
static bool parser_match(Parser* parser, TokenType type);
static bool check_next(Parser* parser, TokenType type);
static char* copy_string(const char* chars, int length);
static char* token_to_string(Token* token);
static Value parse_class_definition(Parser* parser);
static void parse_class_body(Parser* parser, Value class);
static Value parse_method(Parser* parser, Value class, bool is_class_method);
static const char* token_type_to_string(TokenType type);

// Parser functions
static void init_parser(Parser* parser, const char* source, const char* filename) {
    memset(&parser->lexer, 0, sizeof(Lexer));
    parser->lexer.source = source;
    parser->lexer.filename = filename;
    parser->lexer.start = source;
    parser->lexer.current = source;
    parser->lexer.line = 1;
    parser->lexer.column = 0;

    parser->had_error = false;
    parser->panic_mode = false;
    parser->class_index = 0;
    if (DBUG) {
        printf("init_parser\n");
    }

    // Prime the parser with the first token
    advance_token(parser);
}

static void advance_token(Parser* parser) {
    if (DBUG) {
        printf("advance_token: ");
    }
    parser->previous = parser->current;

    for (;;) {
        parser->current = scan_token(&parser->lexer);
        if (DBUG) {
            printf("tok: %s in loop\n", token_type_to_string(parser->current.type));
        }

        if (parser->current.type != TOKEN_ERROR) break;

        error_at_current(parser, parser->current.text);
    }
    if (DBUG) {
        printf("tok: %s\n", token_type_to_string(parser->current.type));
    }
}

static void consume(Parser* parser, TokenType type, const char* message) {
    if (parser->current.type == type) {
        advance_token(parser);
        return;
    }

    error_at_current(parser, message);
}

static bool check(Parser* parser, TokenType type) {
    return parser->current.type == type;
}

static bool parser_match(Parser* parser, TokenType type) {
    if (!check(parser, type)) return false;
    advance_token(parser);
    return true;
}

static bool check_next(Parser* parser, TokenType type) {
    // Skip the check if we're at the end
    if (parser->current.type == TOKEN_EOF) return false;

    // Save current token
    Token saved = parser->current;

    // Advance and check
    advance_token(parser);
    bool result = check(parser, type);

    // Restore current token (hack but works for simple lookahead)
    parser->current = saved;

    return result;
}

// String helpers
static char* copy_string(const char* chars, int length) {
    char* result = (char*)malloc(length + 1);
    memcpy(result, chars, length);
    result[length] = '\0';
    return result;
}

static char* token_to_string(Token* token) {
    return copy_string(token->text, token->length);
}



// SOM class parsing
static Value parse_class_definition(Parser* parser) {
    // Class name
    consume(parser, TOKEN_IDENTIFIER, "Expected class name");
    char* class_name = token_to_string(&parser->previous);

    // Check for existing class
    Value existing = vm_find_class(class_name);
    if (!is_nil(existing)) {
        error_at_previous(parser, "Class already exists");
        free(class_name);
        return vm->nil;
    }

    // Superclass
    Value superclass = vm->class_Object; // Default to Object

    if (parser_match(parser, TOKEN_OPERATOR) &&
        parser->previous.length == 1 &&
        parser->previous.text[0] == '=') {

        consume(parser, TOKEN_IDENTIFIER, "Expected superclass name");
        char* superclass_name = token_to_string(&parser->previous);
        if (DBUG) {
            printf("superclass is %s\n", superclass_name);
        }

        superclass = vm_find_class(superclass_name);
        if (DBUG) {
            printf("superclass is %s\n", superclass);
        }
        if (is_nil(superclass)) {
            error_at_previous(parser, "Unknown superclass");
            free(superclass_name);
            free(class_name);
            return vm->nil;
        }

        free(superclass_name);
    }

    // Start of class body
    consume(parser, TOKEN_LPAREN, "Expected '(' after class declaration");

    // Create the class
    Class* new_class = class_new(class_name, superclass, 0);
    Value class = make_object((Object*)new_class);
    free(class_name);

    // Add class to globals
    vm->globals[parser->class_index++] = class;

    // Instance variables
    if (parser_match(parser, TOKEN_SEPARATOR) &&
        parser->previous.length == 1 &&
        parser->previous.text[0] == '|') {

        // Parse instance variables
        while (!check(parser, TOKEN_SEPARATOR)) {
            consume(parser, TOKEN_IDENTIFIER, "Expected instance variable name");
            // Currently we don't track instance variable names,
            // but we could store them in the class if needed

            if (check(parser, TOKEN_SEPARATOR)) break;
            // No need for a separator between variables
        }

        consume(parser, TOKEN_SEPARATOR, "Expected '|' after instance variables");
    }

    // Method definitions
    parse_class_body(parser, class);

    // End of class body
    consume(parser, TOKEN_RPAREN, "Expected ')' after class body");

    return class;
}

static void parse_class_body(Parser* parser, Value class) {
    // Create array for methods
    Value methods = array_new(0);
    Class* class_obj = (Class*)as_object(class);
    class_obj->methods = methods;

    while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_RPAREN)) {
        // Method type
        bool is_class_method = false;

        if (parser_match(parser, TOKEN_IDENTIFIER)) {
            if (parser->previous.length == 4 &&
                strncmp(parser->previous.text, "class", 4) == 0) {
                is_class_method = true;
            } else {
                error_at_previous(parser, "Expected 'class' or method definition");
                return;
            }
        }

        // Parse the method
        Value method = parse_method(parser, class, is_class_method);

        if (!is_nil(method)) {
            // Add method to class
            Value current_methods = class_obj->methods;
            int size = as_object(current_methods)->size;

            Value new_methods = array_new(size + 1);

            // Copy existing methods
            for (int i = 0; i < size; i++) {
                array_at_put(new_methods, i, array_at(current_methods, i));
            }

            // Add new method
            array_at_put(new_methods, size, method);
            class_obj->methods = new_methods;
        }
    }
}

static Value parse_method(Parser* parser, Value class, bool is_class_method) {
    // Method signature
    Value selector;
    int num_args = 0;

    if (parser_match(parser, TOKEN_IDENTIFIER)) {
        // Unary method
        char* name = token_to_string(&parser->previous);
        selector = symbol_for(name);
        free(name);
    } else if (parser_match(parser, TOKEN_OPERATOR)) {
        // Binary method
        char* name = token_to_string(&parser->previous);
        selector = symbol_for(name);
        free(name);

        consume(parser, TOKEN_IDENTIFIER, "Expected argument name after binary operator");
        num_args = 1;
    } else if (parser_match(parser, TOKEN_KEYWORD)) {
        // Keyword method
        char* name = token_to_string(&parser->previous);
        int name_length = strlen(name);
        name[name_length - 1] = '\0'; // Remove trailing colon

        // Start building the selector
        char* selector_name = strdup(name);
        free(name);

        consume(parser, TOKEN_IDENTIFIER, "Expected argument name after keyword");
        num_args = 1;

        // Handle additional keyword parts
        while (parser_match(parser, TOKEN_KEYWORD)) {
            char* next_part = token_to_string(&parser->previous);

            // Append to selector
            int old_length = strlen(selector_name);
            int part_length = strlen(next_part);
            selector_name = realloc(selector_name, old_length + part_length + 1);
            strcat(selector_name, next_part);
            free(next_part);

            consume(parser, TOKEN_IDENTIFIER, "Expected argument name after keyword");
            num_args++;
        }

        selector = symbol_for(selector_name);
        free(selector_name);
    } else {
        error_at_current(parser, "Expected method selector");
        return vm->nil;
    }

    // Method body
    consume(parser, TOKEN_SEPARATOR, "Expected '|' or method body");

    int num_locals = 0;

    // Local variables
    if (parser->previous.length == 1 && parser->previous.text[0] == '|') {
        while (!check(parser, TOKEN_SEPARATOR)) {
            consume(parser, TOKEN_IDENTIFIER, "Expected local variable name");
            num_locals++;

            if (check(parser, TOKEN_SEPARATOR)) break;
            // No need for a separator between variables
        }

        consume(parser, TOKEN_SEPARATOR, "Expected '|' after local variables");
    }

    // Create method object
    Method* method = method_new(symbol_to_string(selector), num_args, num_locals);
    method->holder = class;

    // Check for primitive
    if (parser_match(parser, TOKEN_PRIMITIVE)) {
        // Parse primitive number
        char* primitive_str = token_to_string(&parser->previous);
        int primitive_id = atoi(primitive_str);
        free(primitive_str);

        // Add bytecode for primitive call
        method->bytecode[0] = BC_PRIMITIVE;
        method->bytecode[1] = (uint8_t)primitive_id;
        method->bytecode[2] = (uint8_t)num_args;
        method->bytecode[3] = BC_RETURN_LOCAL;
        method->bytecode_count = 4;
    } else {
        // Parse method body - this would be a complex task
        // For simplicity, we'll just generate a basic "return self" method
                method->bytecode[0] = BC_PUSH_THIS;
                method->bytecode[1] = BC_RETURN_LOCAL;
                method->bytecode_count = 2;

        // Skip to the end of the method (for now)
        // Look for the method end period
        while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_PERIOD)) {
            advance_token(parser);
        }

        // Consume the period at the end of the method
        if (check(parser, TOKEN_PERIOD)) {
            advance_token(parser);
        }
    }

    return make_object((Object*)method);
}

// Public functions
bool parse_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", filename);
        return false;
    }

    // Get file size
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    if (DBUG) {
        printf("parse_file: %s %d\n", filename, file_size);
    }
    // Read the file
    char* source = (char*)malloc(file_size + 1);
    if (source == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", filename);
        fclose(file);
        return false;
    }

    size_t bytes_read = fread(source, sizeof(char), file_size, file);
    source[bytes_read] = '\0';
    if (DBUG) {
        printf("parse_file: read: %d =? %d\n", bytes_read, file_size);
    }

    fclose(file);

    bool result = parse_string(source, filename);

    free(source);
    return result;
}

bool parse_string(const char* source, const char* name) {
    Parser parser;
    init_parser(&parser, source, name);
    if (DBUG) {
        printf("parse_string: after init\n");
    }


    // Parse the class definition
    Value class = parse_class_definition(&parser);

    return !parser.had_error;
}

bool parser_had_error() {
    // This would track global parser state
    return false; // For now always return false
}

void parser_reset_error() {
    // Reset global error state
}

void print_tokens(const char* source) {
    Lexer lexer;
    lexer.source = source;
    lexer.filename = "<debug>";
    lexer.start = source;
    lexer.current = source;
    lexer.line = 1;
    lexer.column = 0;
    lexer.had_error = false;
    lexer.panic_mode = false;

    for (;;) {
        Token token = scan_token(&lexer);

        printf("%2d:%-2d %-12s '%.*s'\n",
               token.line, token.column,
               token_type_to_string(token.type),
               token.length, token.text);

        if (token.type == TOKEN_EOF) break;
    }
}

// Debug helper functions
static const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_KEYWORD: return "KEYWORD";
        case TOKEN_INTEGER: return "INTEGER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_SYMBOL: return "SYMBOL";
        case TOKEN_OPERATOR: return "OPERATOR";
        case TOKEN_SEPARATOR: return "SEPARATOR";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_CARET: return "CARET";
        case TOKEN_COLON: return "COLON";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_PERIOD: return "PERIOD";
        case TOKEN_PRIMITIVE: return "PRIMITIVE";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
