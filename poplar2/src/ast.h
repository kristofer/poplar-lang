// ast.h - Abstract Syntax Tree structures for SOM

#ifndef POPLAR2_AST_H
#define POPLAR2_AST_H

#include "value.h"
#include <stdbool.h>

// AST node types
typedef enum {
    AST_LITERAL,      // Integer, String, Symbol, etc.
    AST_VARIABLE,     // Variable reference
    AST_ASSIGNMENT,   // Variable assignment
    AST_RETURN,       // Return statement
    AST_MESSAGE_SEND, // Method call
    AST_BLOCK,        // Code block
    AST_SEQUENCE      // Sequence of statements
} AstNodeType;

// Message types
typedef enum {
    MESSAGE_UNARY,    // No arguments
    MESSAGE_BINARY,   // One argument, operator-like syntax
    MESSAGE_KEYWORD   // One or more arguments with keywords
} MessageType;

// Forward declaration of AST node
typedef struct AstNode AstNode;

// Message send node
typedef struct {
    MessageType type;  // Unary, binary, keyword
    Value selector;    // Symbol for message name
    AstNode* receiver; // Receiver object
    int arg_count;     // Number of arguments
    AstNode** args;    // Array of argument expressions
} MessageSendNode;

// Variable node
typedef struct {
    Value name;       // Symbol for variable name
    enum {
        VAR_LOCAL,    // Local variable
        VAR_ARGUMENT, // Method argument
        VAR_INSTANCE, // Instance variable
        VAR_GLOBAL    // Global variable
    } scope;
    int index;        // Index in appropriate scope
} VariableNode;

// Assignment node
typedef struct {
    VariableNode variable; // Target variable
    AstNode* value;        // Value expression
} AssignmentNode;

// Block node
typedef struct {
    int arg_count;     // Number of arguments
    Value* arg_names;  // Array of argument names (symbols)
    AstNode* body;     // Block body
} BlockNode;

// Sequence node
typedef struct {
    int count;         // Number of statements
    AstNode** statements; // Array of statements
} SequenceNode;

// AST node (union of all node types)
struct AstNode {
    AstNodeType type;
    union {
        Value literal;           // AST_LITERAL
        VariableNode variable;   // AST_VARIABLE
        AssignmentNode assign;   // AST_ASSIGNMENT
        AstNode* return_expr;    // AST_RETURN
        MessageSendNode message; // AST_MESSAGE_SEND
        BlockNode block;         // AST_BLOCK
        SequenceNode sequence;   // AST_SEQUENCE
    };
};

// AST node creation functions
AstNode* ast_create_literal(Value literal);
AstNode* ast_create_variable(Value name, int scope, int index);
AstNode* ast_create_assignment(VariableNode variable, AstNode* value);
AstNode* ast_create_return(AstNode* expr);
AstNode* ast_create_message_send(MessageType type, Value selector, AstNode* receiver, int arg_count, AstNode** args);
AstNode* ast_create_block(int arg_count, Value* arg_names, AstNode* body);
AstNode* ast_create_sequence(int count, AstNode** statements);

// Free AST resources
void ast_free(AstNode* node);

// Print AST for debugging
void ast_print(AstNode* node, int indent);

#endif /* POPLAR2_AST_H */
