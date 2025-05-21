// som_parser.c - SOM language parser implementation for Poplar2

#include "som_parser.h"
#include "vm.h"
#include "ast.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Forward declarations for expression parsing
static AstNode* parse_expression(Parser* parser);
static AstNode* parse_primary(Parser* parser);
static AstNode* parse_message_send(Parser* parser, AstNode* receiver);
static AstNode* parse_unary_message(Parser* parser, AstNode* receiver);
static AstNode* parse_binary_message(Parser* parser, AstNode* receiver);
static AstNode* parse_keyword_message(Parser* parser, AstNode* receiver);
static AstNode* parse_block(Parser* parser);
static AstNode* parse_array_literal(Parser* parser);
static AstNode* parse_assignment(Parser* parser, Value var_name, int var_scope, int var_index);
static AstNode* parse_cascade(Parser* parser, AstNode* receiver);

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

    while (!is_at_end(lexer)) {
        // Check for escaped single quote (two consecutive single quotes)
        if (peek(lexer) == '\'' && peek_next(lexer) == '\'') {
            // Advance past both quotes (including one in the string)
            advance(lexer);
            advance(lexer);
            continue;
        }

        // Check for closing quote
        if (peek(lexer) == '\'') {
            break;
        }

        // Check for unterminated string
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
    } else {
        return error_token(lexer, "Unterminated string");
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
        if (parser->current.type != TOKEN_ERROR) break;

        error_at_current(parser, parser->current.text);
    }
    if (DBUG) {
        printf("tok: %s %s\n", token_type_to_string(parser->current.type),
            parser->current.text);
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
            printf("superclass is %d\n", (int)superclass.value);
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
    // Method signature (same as before)
    Value selector;
    int num_args = 0;
    Value* arg_names = NULL;

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
        char* arg_name = token_to_string(&parser->previous);

        arg_names = malloc(sizeof(Value) * 1);
        arg_names[0] = symbol_for(arg_name);
        free(arg_name);

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
        char* arg_name = token_to_string(&parser->previous);

        // Initialize arg_names
        arg_names = malloc(sizeof(Value) * 1);
        arg_names[0] = symbol_for(arg_name);
        free(arg_name);

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
            char* next_arg_name = token_to_string(&parser->previous);

            // Add to arg_names
            arg_names = realloc(arg_names, sizeof(Value) * (num_args + 1));
            arg_names[num_args] = symbol_for(next_arg_name);
            free(next_arg_name);

            num_args++;
        }

        selector = symbol_for(selector_name);
        free(selector_name);
    } else {
        error_at_current(parser, "Expected method selector");
        return vm->nil;
    }

    // Consume equals sign that begins the method body
    consume(parser, TOKEN_OPERATOR, "Expected '=' after method name");

    // Make sure it's actually an equals sign
    if (parser->previous.length != 1 || parser->previous.text[0] != '=') {
        error_at_previous(parser, "Expected '=' after method name");
        return vm->nil;
    }

    // Consume opening parenthesis
    consume(parser, TOKEN_LPAREN, "Expected '(' after '='");

    int num_locals = 0;
    Value* local_names = NULL;

    // Local variables (if present)
    if (parser_match(parser, TOKEN_SEPARATOR) &&
        parser->previous.length == 1 &&
        parser->previous.text[0] == '|') {

        // Count local variables first
        int local_count = 0;
        Token saved_current = parser->current;

        while (!check(parser, TOKEN_SEPARATOR)) {
            if (check(parser, TOKEN_IDENTIFIER)) {
                local_count++;
                advance_token(parser);
            } else {
                break;
            }
        }

        // Restore parser position
        parser->current = saved_current;

        // Allocate array for local variable names
        local_names = local_count > 0 ? malloc(sizeof(Value) * local_count) : NULL;
        num_locals = local_count;

        // Now parse the local variables again
        int i = 0;
        while (!check(parser, TOKEN_SEPARATOR)) {
            consume(parser, TOKEN_IDENTIFIER, "Expected local variable name");
            char* name = token_to_string(&parser->previous);
            local_names[i++] = symbol_for(name);
            free(name);

            if (check(parser, TOKEN_SEPARATOR)) break;
        }

        consume(parser, TOKEN_SEPARATOR, "Expected '|' after local variables");
    }

    // Create method object
    Method* method = method_new(symbol_to_string(selector), num_args, num_locals);
    method->holder = class;

    // Parse method body
    if (parser_match(parser, TOKEN_PRIMITIVE)) {
        // Parse primitive number (same as before)
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
        // Parse method body as expressions
        AstNode** statements = NULL;
        int statement_count = 0;

        // Parse statements until we reach the closing paren
        while (!check(parser, TOKEN_RPAREN) && !check(parser, TOKEN_EOF)) {
            AstNode* stmt = parse_expression(parser);

            // Add the statement to our array
            statements = realloc(statements, sizeof(AstNode*) * (statement_count + 1));
            statements[statement_count++] = stmt;

            // Optional period after statement
            parser_match(parser, TOKEN_PERIOD);
        }

        // Create sequence from statements
        AstNode* body = ast_create_sequence(statement_count, statements);

        // For now, just print the AST for debugging
        if (DBUG) {
            printf("Method AST for %s:\n", symbol_to_string(selector));
            ast_print(body, 2);
        }

        // Create scope info for code generation
        ScopeInfo scope;
        scope.arg_names = arg_names;
        scope.num_args = num_args;
        scope.local_names = local_names;
        scope.num_locals = num_locals;
        scope.instance_names = NULL; // We don't know instance variables yet
        scope.num_instances = 0;     // We'll need to add instance variable support later

        // Generate bytecode from AST
        int code_index = 0;
        code_index = generate_bytecode(method, body, &scope, code_index);
        method->bytecode_count = code_index;

        // If no explicit return, add implicit return self
        if (statement_count == 0 || body->sequence.statements[statement_count-1]->type != AST_RETURN) {
            if (code_index + 2 < MAX_BYTECODE_SIZE) {
                method->bytecode[code_index++] = BC_PUSH_THIS;
                method->bytecode[code_index++] = BC_RETURN_LOCAL;
                method->bytecode_count = code_index;
            }
        }

        // Free the AST
        ast_free(body);
    }

    // Consume the closing parenthesis
    consume(parser, TOKEN_RPAREN, "Expected ')' at end of method");

    // Free resources
    free(arg_names);
    free(local_names);

    return make_object((Object*)method);
}

// static Value parse_method(Parser* parser, Value class, bool is_class_method) {
//     // Method signature (same as before)
//     Value selector;
//     int num_args = 0;

//     if (parser_match(parser, TOKEN_IDENTIFIER)) {
//         // Unary method
//         char* name = token_to_string(&parser->previous);
//         selector = symbol_for(name);
//         free(name);
//     } else if (parser_match(parser, TOKEN_OPERATOR)) {
//         // Binary method
//         char* name = token_to_string(&parser->previous);
//         selector = symbol_for(name);
//         free(name);

//         consume(parser, TOKEN_IDENTIFIER, "Expected argument name after binary operator");
//         num_args = 1;
//     } else if (parser_match(parser, TOKEN_KEYWORD)) {
//         // Keyword method
//         char* name = token_to_string(&parser->previous);
//         int name_length = strlen(name);
//         name[name_length - 1] = '\0'; // Remove trailing colon

//         // Start building the selector
//         char* selector_name = strdup(name);
//         free(name);

//         consume(parser, TOKEN_IDENTIFIER, "Expected argument name after keyword");
//         num_args = 1;

//         // Handle additional keyword parts
//         while (parser_match(parser, TOKEN_KEYWORD)) {
//             char* next_part = token_to_string(&parser->previous);

//             // Append to selector
//             int old_length = strlen(selector_name);
//             int part_length = strlen(next_part);
//             selector_name = realloc(selector_name, old_length + part_length + 1);
//             strcat(selector_name, next_part);
//             free(next_part);

//             consume(parser, TOKEN_IDENTIFIER, "Expected argument name after keyword");
//             num_args++;
//         }

//         selector = symbol_for(selector_name);
//         free(selector_name);
//     } else {
//         error_at_current(parser, "Expected method selector");
//         return vm->nil;
//     }

//     // Consume equals sign that begins the method body
//     consume(parser, TOKEN_OPERATOR, "Expected '=' after method name");

//     // Make sure it's actually an equals sign
//     if (parser->previous.length != 1 || parser->previous.text[0] != '=') {
//         error_at_previous(parser, "Expected '=' after method name");
//         return vm->nil;
//     }

//     // Consume opening parenthesis
//     consume(parser, TOKEN_LPAREN, "Expected '(' after '='");

//     // Store variable names for arguments and locals
//     Value* arg_names = num_args > 0 ? malloc(sizeof(Value) * num_args) : NULL;
//     int num_locals = 0;
//     Value* local_names = NULL;

//     // Collect argument names - this would need to be implemented

//     // Local variables (if present)
//     if (parser_match(parser, TOKEN_SEPARATOR) &&
//         parser->previous.length == 1 &&
//         parser->previous.text[0] == '|') {

//         // Count local variables first
//         int local_count = 0;
//         Token saved_current = parser->current;

//         while (!check(parser, TOKEN_SEPARATOR)) {
//             if (check(parser, TOKEN_IDENTIFIER)) {
//                 local_count++;
//                 advance_token(parser);
//             } else {
//                 break;
//             }
//         }

//         // Restore parser position
//         parser->current = saved_current;

//         // Allocate array for local variable names
//         local_names = local_count > 0 ? malloc(sizeof(Value) * local_count) : NULL;
//         num_locals = local_count;

//         // Now parse the local variables again
//         int i = 0;
//         while (!check(parser, TOKEN_SEPARATOR)) {
//             consume(parser, TOKEN_IDENTIFIER, "Expected local variable name");
//             char* name = token_to_string(&parser->previous);
//             local_names[i++] = symbol_for(name);
//             free(name);

//             if (check(parser, TOKEN_SEPARATOR)) break;
//         }

//         consume(parser, TOKEN_SEPARATOR, "Expected '|' after local variables");
//     }

//     // Create method object
//     Method* method = method_new(symbol_to_string(selector), num_args, num_locals);
//     method->holder = class;

//     // Parse method body - NEW CODE HERE!
//     if (parser_match(parser, TOKEN_PRIMITIVE)) {
//         // Parse primitive number (same as before)
//         char* primitive_str = token_to_string(&parser->previous);
//         int primitive_id = atoi(primitive_str);
//         free(primitive_str);

//         // Add bytecode for primitive call
//         method->bytecode[0] = BC_PRIMITIVE;
//         method->bytecode[1] = (uint8_t)primitive_id;
//         method->bytecode[2] = (uint8_t)num_args;
//         method->bytecode[3] = BC_RETURN_LOCAL;
//         method->bytecode_count = 4;
//     } else {
//         // Parse method body as expressions
//         AstNode* body = NULL;

//         // Create a sequence node to hold all statements
//         AstNode** statements = NULL;
//         int statement_count = 0;

//         // Parse statements until we reach the closing paren
//         while (!check(parser, TOKEN_RPAREN) && !check(parser, TOKEN_EOF)) {
//             AstNode* stmt = parse_expression(parser);

//             // Add the statement to our array
//             statements = realloc(statements, sizeof(AstNode*) * (statement_count + 1));
//             statements[statement_count++] = stmt;

//             // Optional period after statement
//             parser_match(parser, TOKEN_PERIOD);
//         }

//         // Create sequence from statements
//         body = ast_create_sequence(statement_count, statements);

//         // For now, just print the AST for debugging
//         ast_print(body, 0);

//         // We'll replace this with real bytecode generation in the next step
//         // For now, just return self
//         method->bytecode[0] = BC_PUSH_THIS;
//         method->bytecode[1] = BC_RETURN_LOCAL;
//         method->bytecode_count = 2;

//         // Free the AST
//         ast_free(body);
//     }

//     // Consume the closing parenthesis
//     consume(parser, TOKEN_RPAREN, "Expected ')' at end of method");

//     // Free resources
//     free(arg_names);
//     free(local_names);

//     return make_object((Object*)method);
// }

// Basic implementation of expression parsing - to be expanded
// static AstNode* parse_expression(Parser* parser) {
//     // Check for return statement
//     if (parser_match(parser, TOKEN_CARET)) {
//         AstNode* expr = parse_expression(parser);
//         return ast_create_return(expr);
//     }

//     // Parse primary expression
//     AstNode* expr = parse_primary(parser);

//     // Look for message sends
//     if (check(parser, TOKEN_IDENTIFIER) || check(parser, TOKEN_OPERATOR) || check(parser, TOKEN_KEYWORD)) {
//         expr = parse_message_send(parser, expr);
//     }

//     return expr;
// }

// static AstNode* parse_primary(Parser* parser) {
//     // Parse literals, variables, and other primary expressions
//     if (parser_match(parser, TOKEN_INTEGER)) {
//         char* value_str = token_to_string(&parser->previous);
//         int value = atoi(value_str);
//         free(value_str);
//         return ast_create_literal(make_int(value));
//     }

//     if (parser_match(parser, TOKEN_STRING)) {
//         char* value_str = token_to_string(&parser->previous);
//         Value string_val = string_new(value_str);
//         free(value_str);
//         return ast_create_literal(string_val);
//     }

//     if (parser_match(parser, TOKEN_IDENTIFIER)) {
//         // This is a simple variable reference
//         char* name = token_to_string(&parser->previous);
//         Value var_name = symbol_for(name);
//         free(name);

//         // For now, assume it's a local variable with index 0
//         // Later, we'll need to look up the variable in the current scope
//         return ast_create_variable(var_name, VAR_LOCAL, 0);
//     }

//     error_at_current(parser, "Expected expression");
//     return NULL;
// }

// static AstNode* parse_message_send(Parser* parser, AstNode* receiver) {
//     // Parse different types of message sends
//     if (check(parser, TOKEN_IDENTIFIER) && !check_next(parser, TOKEN_COLON)) {
//         return parse_unary_message(parser, receiver);
//     }

//     if (check(parser, TOKEN_OPERATOR)) {
//         return parse_binary_message(parser, receiver);
//     }

//     if (check(parser, TOKEN_KEYWORD)) {
//         return parse_keyword_message(parser, receiver);
//     }

//     error_at_current(parser, "Expected message send");
//     return NULL;
// }

// static AstNode* parse_unary_message(Parser* parser, AstNode* receiver) {
//     consume(parser, TOKEN_IDENTIFIER, "Expected unary message name");

//     char* name = token_to_string(&parser->previous);
//     Value selector = symbol_for(name);
//     free(name);

//     return ast_create_message_send(MESSAGE_UNARY, selector, receiver, 0, NULL);
// }

// static AstNode* parse_binary_message(Parser* parser, AstNode* receiver) {
//     consume(parser, TOKEN_OPERATOR, "Expected binary operator");

//     char* op = token_to_string(&parser->previous);
//     Value selector = symbol_for(op);
//     free(op);

//     // Parse the argument
//     AstNode* arg = parse_primary(parser);

//     // Create args array with the single argument
//     AstNode** args = malloc(sizeof(AstNode*));
//     args[0] = arg;

//     return ast_create_message_send(MESSAGE_BINARY, selector, receiver, 1, args);
// }

// static AstNode* parse_keyword_message(Parser* parser, AstNode* receiver) {
//     // Start building the selector name
//     char* selector_name = NULL;
//     AstNode** args = NULL;
//     int arg_count = 0;

//     // Parse all keyword parts
//     do {
//         consume(parser, TOKEN_KEYWORD, "Expected keyword");

//         // Get the keyword part
//         char* keyword_part = token_to_string(&parser->previous);

//         // Add to selector name
//         if (selector_name == NULL) {
//             selector_name = keyword_part;
//         } else {
//             int old_length = strlen(selector_name);
//             int part_length = strlen(keyword_part);
//             selector_name = realloc(selector_name, old_length + part_length + 1);
//             strcat(selector_name, keyword_part);
//             free(keyword_part);
//         }

//         // Parse the argument
//         AstNode* arg = parse_primary(parser);

//         // Add to args array
//         args = realloc(args, sizeof(AstNode*) * (arg_count + 1));
//         args[arg_count++] = arg;
//     } while (check(parser, TOKEN_KEYWORD));

//     // Create the message send node
//     Value selector = symbol_for(selector_name);
//     free(selector_name);

//     return ast_create_message_send(MESSAGE_KEYWORD, selector, receiver, arg_count, args);
// }


// Parse a full expression
static AstNode* parse_expression(Parser* parser) {
    // Check for return statement
    if (parser_match(parser, TOKEN_CARET)) {
        AstNode* expr = parse_expression(parser);
        return ast_create_return(expr);
    }

    // Parse primary expression
    AstNode* expr = parse_primary(parser);

    // Look for message sends, cascades, or assignments
    if (expr != NULL && expr->type == AST_VARIABLE) {
        // Check for assignment (:=)
        if (check(parser, TOKEN_ASSIGN)) {
            advance_token(parser); // Consume :=
            return parse_assignment(parser, expr->variable.name, expr->variable.scope, expr->variable.index);
        }
    }

    // Message sends
    if (check(parser, TOKEN_IDENTIFIER) || check(parser, TOKEN_OPERATOR) || check(parser, TOKEN_KEYWORD)) {
        expr = parse_message_send(parser, expr);

        // Check for cascade
        if (check(parser, TOKEN_SEPARATOR) && parser->current.length == 1 && parser->current.text[0] == ';') {
            expr = parse_cascade(parser, expr);
        }
    }

    return expr;
}

// Parse a cascade expression (message chaining with semicolons)
static AstNode* parse_cascade(Parser* parser, AstNode* receiver) {
    // Create sequence to hold all messages in cascade
    AstNode** statements = malloc(sizeof(AstNode*) * 1);
    statements[0] = receiver;  // First message is already parsed
    int statement_count = 1;

    while (parser_match(parser, TOKEN_SEPARATOR) &&
           parser->previous.length == 1 &&
           parser->previous.text[0] == ';') {

        // Parse the next message with the same receiver
        AstNode* message = NULL;

        if (check(parser, TOKEN_IDENTIFIER) && !check_next(parser, TOKEN_COLON)) {
            // Clone receiver for each message
            AstNode* receiver_clone = ast_create_variable(
                receiver->type == AST_VARIABLE ? receiver->variable.name : vm->nil,
                receiver->type == AST_VARIABLE ? receiver->variable.scope : 0,
                receiver->type == AST_VARIABLE ? receiver->variable.index : 0
            );

            message = parse_unary_message(parser, receiver_clone);
        } else if (check(parser, TOKEN_OPERATOR)) {
            AstNode* receiver_clone = ast_create_variable(
                receiver->type == AST_VARIABLE ? receiver->variable.name : vm->nil,
                receiver->type == AST_VARIABLE ? receiver->variable.scope : 0,
                receiver->type == AST_VARIABLE ? receiver->variable.index : 0
            );

            message = parse_binary_message(parser, receiver_clone);
        } else if (check(parser, TOKEN_KEYWORD)) {
            AstNode* receiver_clone = ast_create_variable(
                receiver->type == AST_VARIABLE ? receiver->variable.name : vm->nil,
                receiver->type == AST_VARIABLE ? receiver->variable.scope : 0,
                receiver->type == AST_VARIABLE ? receiver->variable.index : 0
            );

            message = parse_keyword_message(parser, receiver_clone);
        } else {
            error_at_current(parser, "Expected message selector after cascade");
            break;
        }

        // Add to sequence
        statements = realloc(statements, sizeof(AstNode*) * (statement_count + 1));
        statements[statement_count++] = message;
    }

    return ast_create_sequence(statement_count, statements);
}

// Parse a cascade expression (message chaining with semicolons)
// static AstNode* parse_cascade(Parser* parser, AstNode* receiver) {
//     // Create sequence to hold all messages in cascade
//     AstNode** statements = malloc(sizeof(AstNode*) * 1);
//     statements[0] = receiver;  // First message is already parsed
//     int statement_count = 1;

//     while (parser_match(parser, TOKEN_SEPARATOR) &&
//            parser->previous.length == 1 &&
//            parser->previous.text[0] == ';') {

//         // Parse the next message with the same receiver
//         AstNode* message = NULL;

//         if (check(parser, TOKEN_IDENTIFIER) && !check_next(parser, TOKEN_COLON)) {
//             // Clone receiver for each message
//             AstNode* receiver_clone = ast_create_variable(
//                 receiver->type == AST_VARIABLE ? receiver->variable.name : make_nil(),
//                 receiver->type == AST_VARIABLE ? receiver->variable.scope : 0,
//                 receiver->type == AST_VARIABLE ? receiver->variable.index : 0
//             );

//             message = parse_unary_message(parser, receiver_clone);
//         } else if (check(parser, TOKEN_OPERATOR)) {
//             AstNode* receiver_clone = ast_create_variable(
//                 receiver->type == AST_VARIABLE ? receiver->variable.name : make_nil(),
//                 receiver->type == AST_VARIABLE ? receiver->variable.scope : 0,
//                 receiver->type == AST_VARIABLE ? receiver->variable.index : 0
//             );

//             message = parse_binary_message(parser, receiver_clone);
//         } else if (check(parser, TOKEN_KEYWORD)) {
//             AstNode* receiver_clone = ast_create_variable(
//                 receiver->type == AST_VARIABLE ? receiver->variable.name : make_nil(),
//                 receiver->type == AST_VARIABLE ? receiver->variable.scope : 0,
//                 receiver->type == AST_VARIABLE ? receiver->variable.index : 0
//             );

//             message = parse_keyword_message(parser, receiver_clone);
//         } else {
//             error_at_current(parser, "Expected message selector after cascade");
//             break;
//         }

//         // Add to sequence
//         statements = realloc(statements, sizeof(AstNode*) * (statement_count + 1));
//         statements[statement_count++] = message;
//     }

//     return ast_create_sequence(statement_count, statements);
// }

// Parse an assignment expression
static AstNode* parse_assignment(Parser* parser, Value var_name, int var_scope, int var_index) {
    // Parse the value to be assigned
    AstNode* value = parse_expression(parser);

    // Create variable node
    VariableNode variable;
    variable.name = var_name;
    variable.scope = var_scope;
    variable.index = var_index;

    return ast_create_assignment(variable, value);
}

static AstNode* parse_primary(Parser* parser) {
    // Parse literals, variables, blocks, arrays, and other primary expressions

    // Literals
    if (parser_match(parser, TOKEN_INTEGER)) {
        char* value_str = token_to_string(&parser->previous);
        int value = atoi(value_str);
        free(value_str);
        return ast_create_literal(make_int(value));
    }

    if (parser_match(parser, TOKEN_STRING)) {
        char* value_str = token_to_string(&parser->previous);
        Value string_val = string_new(value_str);
        free(value_str);
        return ast_create_literal(string_val);
    }

    // Special literals
    if (parser_match(parser, TOKEN_IDENTIFIER)) {
        char* name = token_to_string(&parser->previous);

        // Handle special constants
        if (strcmp(name, "nil") == 0) {
            free(name);
            return ast_create_literal(vm->nil);
        } else if (strcmp(name, "true") == 0) {
            free(name);
            return ast_create_literal(vm->true_obj);
        } else if (strcmp(name, "false") == 0) {
            free(name);
            return ast_create_literal(vm->false_obj);
        } else if (strcmp(name, "self") == 0) {
            free(name);
            // Special 'self' variable (receiver)
            return ast_create_variable(symbol_for("self"), VAR_ARGUMENT, -1);
        } else if (strcmp(name, "super") == 0) {
            free(name);
            // Special 'super' variable
            return ast_create_variable(symbol_for("super"), VAR_ARGUMENT, -2);
        } else {
            // This is a regular variable reference
            Value var_name = symbol_for(name);
            free(name);

            // For now, assume it's a local variable with index 0
            // Later, we'll need to look up the variable in the current scope
            return ast_create_variable(var_name, VAR_LOCAL, 0);
        }
    }

    // Symbol literals
    if (parser_match(parser, TOKEN_SYMBOL)) {
        char* name = token_to_string(&parser->previous);
        Value symbol = symbol_for(name);
        free(name);
        return ast_create_literal(symbol);
    }

    // Block expression [...]
    if (parser_match(parser, TOKEN_LBRACKET)) {
        return parse_block(parser);
    }

    // Array literal #(...)
    if (parser_match(parser, TOKEN_SYMBOL) &&
        check(parser, TOKEN_LPAREN)) {
        return parse_array_literal(parser);
    }

    // Parenthesized expression
    if (parser_match(parser, TOKEN_LPAREN)) {
        AstNode* expr = parse_expression(parser);
        consume(parser, TOKEN_RPAREN, "Expected ')' after expression");
        return expr;
    }

    error_at_current(parser, "Expected expression");
    return NULL;
}

// Parse a block expression [...]
static AstNode* parse_block(Parser* parser) {
    // Parse block parameters if any
    Value* arg_names = NULL;
    int arg_count = 0;

    // Check for block parameters
    while (check(parser, TOKEN_COLON)) {
        advance_token(parser); // Consume :
        consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");

        char* name = token_to_string(&parser->previous);
        Value arg_name = symbol_for(name);
        free(name);

        // Add to arg names
        arg_names = realloc(arg_names, sizeof(Value) * (arg_count + 1));
        arg_names[arg_count++] = arg_name;

        // Check for parameter separator |
        if (check(parser, TOKEN_SEPARATOR) &&
            parser->current.length == 1 &&
            parser->current.text[0] == '|') {
            advance_token(parser);
            break;
        }
    }

    // Parse block body
    AstNode* body = NULL;

    // Create a sequence node to hold all statements
    AstNode** statements = NULL;
    int statement_count = 0;

    // Parse statements until we reach the closing bracket
    while (!check(parser, TOKEN_RBRACKET) && !check(parser, TOKEN_EOF)) {
        AstNode* stmt = parse_expression(parser);

        // Add the statement to our array
        statements = realloc(statements, sizeof(AstNode*) * (statement_count + 1));
        statements[statement_count++] = stmt;

        // Optional period after statement
        parser_match(parser, TOKEN_PERIOD);
    }

    // Create sequence from statements
    body = ast_create_sequence(statement_count, statements);

    consume(parser, TOKEN_RBRACKET, "Expected ']' after block");

    return ast_create_block(arg_count, arg_names, body);
}

// Parse an array literal #(...)
static AstNode* parse_array_literal(Parser* parser) {
    consume(parser, TOKEN_LPAREN, "Expected '(' after #");

    // Create a sequence to hold array elements
    AstNode** elements = NULL;
    int element_count = 0;

    // Parse elements until we reach the closing paren
    while (!check(parser, TOKEN_RPAREN) && !check(parser, TOKEN_EOF)) {
        AstNode* element = parse_expression(parser);

        // Add the element to our array
        elements = realloc(elements, sizeof(AstNode*) * (element_count + 1));
        elements[element_count++] = element;
    }

    consume(parser, TOKEN_RPAREN, "Expected ')' after array elements");

    // Create a message send to create the array
    // We'll send #fromElements: with all the elements
    AstNode* array_class = ast_create_variable(symbol_for("Array"), VAR_GLOBAL, 0);
    AstNode** args = malloc(sizeof(AstNode*) * 1);
    args[0] = ast_create_sequence(element_count, elements);

    return ast_create_message_send(
        MESSAGE_KEYWORD,
        symbol_for("fromElements:"),
        array_class,
        1,
        args
    );
}

static AstNode* parse_message_send(Parser* parser, AstNode* receiver) {
    // Parse unary, binary, and keyword messages

    // Start with unary and chain binary/keyword
    AstNode* result = receiver;

    // Parse all unary messages first (highest precedence)
    while (check(parser, TOKEN_IDENTIFIER) &&
           !check_next(parser, TOKEN_COLON)) {
        result = parse_unary_message(parser, result);
    }

    // Then binary messages (middle precedence)
    if (check(parser, TOKEN_OPERATOR)) {
        result = parse_binary_message(parser, result);

        // Chain additional binary messages
        while (check(parser, TOKEN_OPERATOR)) {
            result = parse_binary_message(parser, result);
        }
    }

    // Finally keyword messages (lowest precedence)
    if (check(parser, TOKEN_KEYWORD)) {
        result = parse_keyword_message(parser, result);
    }

    return result;
}

static AstNode* parse_unary_message(Parser* parser, AstNode* receiver) {
    consume(parser, TOKEN_IDENTIFIER, "Expected unary message name");

    char* name = token_to_string(&parser->previous);
    Value selector = symbol_for(name);
    free(name);

    return ast_create_message_send(MESSAGE_UNARY, selector, receiver, 0, NULL);
}

static AstNode* parse_binary_message(Parser* parser, AstNode* receiver) {
    consume(parser, TOKEN_OPERATOR, "Expected binary operator");

    char* op = token_to_string(&parser->previous);
    Value selector = symbol_for(op);
    free(op);

    // Parse the argument (primary expression)
    AstNode* arg = parse_primary(parser);

    // Check for unary messages on the argument
    while (check(parser, TOKEN_IDENTIFIER) &&
           !check_next(parser, TOKEN_COLON) &&
           !check(parser, TOKEN_OPERATOR) &&
           !check(parser, TOKEN_KEYWORD)) {
        arg = parse_unary_message(parser, arg);
    }

    // Create args array with the single argument
    AstNode** args = malloc(sizeof(AstNode*));
    args[0] = arg;

    return ast_create_message_send(MESSAGE_BINARY, selector, receiver, 1, args);
}

static AstNode* parse_keyword_message(Parser* parser, AstNode* receiver) {
    // Start building the selector name
    char* selector_name = NULL;
    AstNode** args = NULL;
    int arg_count = 0;

    // Parse all keyword parts
    do {
        consume(parser, TOKEN_KEYWORD, "Expected keyword");

        // Get the keyword part
        char* keyword_part = token_to_string(&parser->previous);

        // Add to selector name
        if (selector_name == NULL) {
            selector_name = keyword_part;
        } else {
            int old_length = strlen(selector_name);
            int part_length = strlen(keyword_part);
            selector_name = realloc(selector_name, old_length + part_length + 1);
            strcat(selector_name, keyword_part);
            free(keyword_part);
        }

        // Parse the argument (can be a primary followed by unary/binary)
        AstNode* arg = parse_primary(parser);

        // Check for unary messages on the argument
        while (check(parser, TOKEN_IDENTIFIER) &&
               !check_next(parser, TOKEN_COLON) &&
               !check(parser, TOKEN_KEYWORD)) {
            arg = parse_unary_message(parser, arg);
        }

        // Check for binary messages on the argument
        while (check(parser, TOKEN_OPERATOR)) {
            arg = parse_binary_message(parser, arg);
        }

        // Add to args array
        args = realloc(args, sizeof(AstNode*) * (arg_count + 1));
        args[arg_count++] = arg;
    } while (check(parser, TOKEN_KEYWORD));

    // Create the message send node
    Value selector = symbol_for(selector_name);
    free(selector_name);

    return ast_create_message_send(MESSAGE_KEYWORD, selector, receiver, arg_count, args);
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

// Bytecode generation
// Main bytecode generation function
static int generate_bytecode(Method* method, AstNode* node, ScopeInfo* scope, int code_index) {
    switch (node->type) {
        case AST_LITERAL:
            return generate_literal(method, node, code_index);

        case AST_VARIABLE:
            return generate_variable_access(method, node, scope, code_index);

        case AST_ASSIGNMENT:
            return generate_assignment(method, node, scope, code_index);

        case AST_RETURN:
            return generate_return(method, node, scope, code_index);

        case AST_MESSAGE_SEND:
            return generate_message_send(method, node, scope, code_index);

        case AST_BLOCK:
            return generate_block(method, node, scope, code_index);

        case AST_SEQUENCE:
            return generate_sequence(method, node, scope, code_index);

        default:
            vm_error("Unknown AST node type: %d", node->type);
            return code_index;
    }
}

// Generate bytecode for a literal value
static int generate_literal(Method* method, AstNode* node, int code_index) {
    // Find or add to literals table
    int literal_index = -1;

    // Find in literals table
    for (int i = 0; i < MAX_LITERALS; i++) {
        if (value_equals(vm->literals[i], node->literal)) {
            literal_index = i;
            break;
        }
    }

    // If not found, add to literals table
    if (literal_index == -1) {
        // Find first empty slot
        for (int i = 0; i < MAX_LITERALS; i++) {
            if (is_nil(vm->literals[i])) {
                vm->literals[i] = node->literal;
                literal_index = i;
                break;
            }
        }
    }

    // If still not found, literals table is full
    if (literal_index == -1) {
        vm_error("Literals table is full");
        return code_index;
    }

    // Push the literal value
    method->bytecode[code_index++] = BC_PUSH_CONSTANT;
    method->bytecode[code_index++] = (uint8_t)literal_index;

    return code_index;
}

// Generate bytecode for variable access
static int generate_variable_access(Method* method, AstNode* node, ScopeInfo* scope, int code_index) {
    int var_index = -1;
    const char* var_name = symbol_to_string(node->variable.name);

    // Check if it's a special variable
    if (strcmp(var_name, "self") == 0) {
        method->bytecode[code_index++] = BC_PUSH_THIS;
        return code_index;
    }

    if (strcmp(var_name, "nil") == 0) {
        method->bytecode[code_index++] = BC_PUSH_SPECIAL;
        method->bytecode[code_index++] = SPECIAL_NIL;
        return code_index;
    }

    if (strcmp(var_name, "true") == 0) {
        method->bytecode[code_index++] = BC_PUSH_SPECIAL;
        method->bytecode[code_index++] = SPECIAL_TRUE;
        return code_index;
    }

    if (strcmp(var_name, "false") == 0) {
        method->bytecode[code_index++] = BC_PUSH_SPECIAL;
        method->bytecode[code_index++] = SPECIAL_FALSE;
        return code_index;
    }

    // Check local variables
    for (int i = 0; i < scope->num_locals; i++) {
        if (value_equals(scope->local_names[i], node->variable.name)) {
            method->bytecode[code_index++] = BC_PUSH_LOCAL;
            method->bytecode[code_index++] = (uint8_t)i;
            return code_index;
        }
    }

    // Check arguments
    for (int i = 0; i < scope->num_args; i++) {
        if (value_equals(scope->arg_names[i], node->variable.name)) {
            method->bytecode[code_index++] = BC_PUSH_ARGUMENT;
            method->bytecode[code_index++] = (uint8_t)i;
            return code_index;
        }
    }

    // Check instance variables
    for (int i = 0; i < scope->num_instances; i++) {
        if (value_equals(scope->instance_names[i], node->variable.name)) {
            method->bytecode[code_index++] = BC_PUSH_FIELD;
            method->bytecode[code_index++] = (uint8_t)i;
            return code_index;
        }
    }

    // If not found, assume it's a global
    // Find or add to literals table
    int global_index = -1;
    Value name_symbol = node->variable.name;

    for (int i = 0; i < MAX_LITERALS; i++) {
        if (value_equals(vm->literals[i], name_symbol)) {
            global_index = i;
            break;
        }
    }

    if (global_index == -1) {
        for (int i = 0; i < MAX_LITERALS; i++) {
            if (is_nil(vm->literals[i])) {
                vm->literals[i] = name_symbol;
                global_index = i;
                break;
            }
        }
    }

    if (global_index == -1) {
        vm_error("Literals table is full");
        return code_index;
    }

    method->bytecode[code_index++] = BC_PUSH_GLOBAL;
    method->bytecode[code_index++] = (uint8_t)global_index;

    return code_index;
}

// Generate bytecode for an assignment
static int generate_assignment(Method* method, AstNode* node, ScopeInfo* scope, int code_index) {
    // First generate the value to be assigned
    code_index = generate_bytecode(method, node->assign.value, scope, code_index);

    // Duplicate the value on the stack (we'll need it for the return value)
    method->bytecode[code_index++] = BC_DUP;

    int var_index = -1;
    const char* var_name = symbol_to_string(node->assign.variable.name);

    // Check local variables
    for (int i = 0; i < scope->num_locals; i++) {
        if (value_equals(scope->local_names[i], node->assign.variable.name)) {
            method->bytecode[code_index++] = BC_STORE_LOCAL;
            method->bytecode[code_index++] = (uint8_t)i;
            return code_index;
        }
    }

    // Check arguments
    for (int i = 0; i < scope->num_args; i++) {
        if (value_equals(scope->arg_names[i], node->assign.variable.name)) {
            method->bytecode[code_index++] = BC_STORE_ARGUMENT;
            method->bytecode[code_index++] = (uint8_t)i;
            return code_index;
        }
    }

    // Check instance variables
    for (int i = 0; i < scope->num_instances; i++) {
        if (value_equals(scope->instance_names[i], node->assign.variable.name)) {
            method->bytecode[code_index++] = BC_STORE_FIELD;
            method->bytecode[code_index++] = (uint8_t)i;
            return code_index;
        }
    }

    // If not found, assume it's a global
    // Find or add to literals table
    int global_index = -1;
    Value name_symbol = node->assign.variable.name;

    for (int i = 0; i < MAX_LITERALS; i++) {
        if (value_equals(vm->literals[i], name_symbol)) {
            global_index = i;
            break;
        }
    }

    if (global_index == -1) {
        for (int i = 0; i < MAX_LITERALS; i++) {
            if (is_nil(vm->literals[i])) {
                vm->literals[i] = name_symbol;
                global_index = i;
                break;
            }
        }
    }

    if (global_index == -1) {
        vm_error("Literals table is full");
        return code_index;
    }

    method->bytecode[code_index++] = BC_STORE_GLOBAL;
    method->bytecode[code_index++] = (uint8_t)global_index;

    return code_index;
}

// Generate bytecode for a return statement
static int generate_return(Method* method, AstNode* node, ScopeInfo* scope, int code_index) {
    // Generate the return value expression
    code_index = generate_bytecode(method, node->return_expr, scope, code_index);

    // Add return instruction
    method->bytecode[code_index++] = BC_RETURN_LOCAL;

    return code_index;
}

// Generate bytecode for a message send
static int generate_message_send(Method* method, AstNode* node, ScopeInfo* scope, int code_index) {
    // Generate the receiver
    code_index = generate_bytecode(method, node->message.receiver, scope, code_index);

    // Generate each argument
    for (int i = 0; i < node->message.arg_count; i++) {
        code_index = generate_bytecode(method, node->message.args[i], scope, code_index);
    }

    // Find or add the selector to literals table
    int selector_index = -1;

    for (int i = 0; i < MAX_LITERALS; i++) {
        if (value_equals(vm->literals[i], node->message.selector)) {
            selector_index = i;
            break;
        }
    }

    if (selector_index == -1) {
        for (int i = 0; i < MAX_LITERALS; i++) {
            if (is_nil(vm->literals[i])) {
                vm->literals[i] = node->message.selector;
                selector_index = i;
                break;
            }
        }
    }

    if (selector_index == -1) {
        vm_error("Literals table is full");
        return code_index;
    }

    // Check if this is a super send
    if (node->message.receiver->type == AST_VARIABLE &&
        strcmp(symbol_to_string(node->message.receiver->variable.name), "super") == 0) {
        // Super send
        method->bytecode[code_index++] = BC_SUPER_SEND;
    } else {
        // Normal send
        method->bytecode[code_index++] = BC_SEND;
    }

    method->bytecode[code_index++] = (uint8_t)selector_index;
    method->bytecode[code_index++] = (uint8_t)node->message.arg_count;

    return code_index;
}

// Generate bytecode for a block
static int generate_block(Method* method, AstNode* node, ScopeInfo* scope, int code_index) {
    // For now, we'll just create a placeholder for blocks
    // A full implementation would create a block context and compile the block body
    vm_error("Block compilation not yet implemented");

    // Push a nil value as a placeholder
    method->bytecode[code_index++] = BC_PUSH_SPECIAL;
    method->bytecode[code_index++] = SPECIAL_NIL;

    return code_index;
}

// Generate bytecode for a sequence of expressions
static int generate_sequence(Method* method, AstNode* node, ScopeInfo* scope, int code_index) {
    // Generate code for each statement
    for (int i = 0; i < node->sequence.count; i++) {
        code_index = generate_bytecode(method, node->sequence.statements[i], scope, code_index);

        // Pop the result if it's not the last statement
        if (i < node->sequence.count - 1) {
            method->bytecode[code_index++] = BC_POP;
        }
    }

    // If the sequence is empty, push nil
    if (node->sequence.count == 0) {
        method->bytecode[code_index++] = BC_PUSH_SPECIAL;
        method->bytecode[code_index++] = SPECIAL_NIL;
    }

    return code_index;
}
