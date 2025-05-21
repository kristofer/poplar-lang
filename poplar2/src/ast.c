// ast.c - Implementation of AST functions

#include "ast.h"
#include "gc.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>

// AST node creation functions
AstNode* ast_create_literal(Value literal) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    node->type = AST_LITERAL;
    node->literal = literal;
    return node;
}

AstNode* ast_create_variable(Value name, int scope, int index) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    node->type = AST_VARIABLE;
    node->variable.name = name;
    node->variable.scope = scope;
    node->variable.index = index;
    return node;
}

AstNode* ast_create_assignment(VariableNode variable, AstNode* value) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    node->type = AST_ASSIGNMENT;
    node->assign.variable = variable;
    node->assign.value = value;
    return node;
}

AstNode* ast_create_return(AstNode* expr) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    node->type = AST_RETURN;
    node->return_expr = expr;
    return node;
}

AstNode* ast_create_message_send(MessageType type, Value selector, AstNode* receiver, int arg_count, AstNode** args) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    node->type = AST_MESSAGE_SEND;
    node->message.type = type;
    node->message.selector = selector;
    node->message.receiver = receiver;
    node->message.arg_count = arg_count;
    node->message.args = args;
    return node;
}

AstNode* ast_create_block(int arg_count, Value* arg_names, AstNode* body) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    node->type = AST_BLOCK;
    node->block.arg_count = arg_count;
    node->block.arg_names = arg_names;
    node->block.body = body;
    return node;
}

AstNode* ast_create_sequence(int count, AstNode** statements) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    node->type = AST_SEQUENCE;
    node->sequence.count = count;
    node->sequence.statements = statements;
    return node;
}

// Free AST resources
void ast_free(AstNode* node) {
    if (node == NULL) return;

    switch (node->type) {
        case AST_LITERAL:
            // Nothing to free
            break;
        case AST_VARIABLE:
            // Nothing to free
            break;
        case AST_ASSIGNMENT:
            ast_free(node->assign.value);
            break;
        case AST_RETURN:
            ast_free(node->return_expr);
            break;
        case AST_MESSAGE_SEND:
            ast_free(node->message.receiver);
            for (int i = 0; i < node->message.arg_count; i++) {
                ast_free(node->message.args[i]);
            }
            free(node->message.args);
            break;
        case AST_BLOCK:
            free(node->block.arg_names);
            ast_free(node->block.body);
            break;
        case AST_SEQUENCE:
            for (int i = 0; i < node->sequence.count; i++) {
                ast_free(node->sequence.statements[i]);
            }
            free(node->sequence.statements);
            break;
    }

    free(node);
}

// Implementation of AST printing for debugging
void ast_print(AstNode* node, int indent) {
    if (node == NULL) {
        printf("%*sNULL\n", indent, "");
        return;
    }

    switch (node->type) {
        case AST_LITERAL:
            printf("%*sLiteral: ", indent, "");
            value_print(node->literal);
            printf("\n");
            break;

        case AST_VARIABLE: {
            printf("%*sVariable: %s (scope: %d, index: %d)\n",
                indent, "",
                symbol_to_string(node->variable.name),
                node->variable.scope,
                node->variable.index);
            break;
        }

        case AST_ASSIGNMENT:
            printf("%*sAssignment: %s =\n",
                indent, "",
                symbol_to_string(node->assign.variable.name));
            ast_print(node->assign.value, indent + 2);
            break;

        case AST_RETURN:
            printf("%*sReturn:\n", indent, "");
            ast_print(node->return_expr, indent + 2);
            break;

        case AST_MESSAGE_SEND: {
            const char* type_str = "";
            switch (node->message.type) {
                case MESSAGE_UNARY: type_str = "Unary"; break;
                case MESSAGE_BINARY: type_str = "Binary"; break;
                case MESSAGE_KEYWORD: type_str = "Keyword"; break;
            }

            printf("%*sMessage (%s): %s\n",
                indent, "",
                type_str,
                symbol_to_string(node->message.selector));

            printf("%*sReceiver:\n", indent + 2, "");
            ast_print(node->message.receiver, indent + 4);

            for (int i = 0; i < node->message.arg_count; i++) {
                printf("%*sArg %d:\n", indent + 2, "", i + 1);
                ast_print(node->message.args[i], indent + 4);
            }
            break;
        }

        case AST_BLOCK:
            printf("%*sBlock with %d args:\n", indent, "", node->block.arg_count);

            for (int i = 0; i < node->block.arg_count; i++) {
                printf("%*sArg %d: %s\n",
                    indent + 2, "",
                    i + 1,
                    symbol_to_string(node->block.arg_names[i]));
            }

            printf("%*sBody:\n", indent + 2, "");
            ast_print(node->block.body, indent + 4);
            break;

        case AST_SEQUENCE:
            printf("%*sSequence with %d statements:\n", indent, "", node->sequence.count);
            for (int i = 0; i < node->sequence.count; i++) {
                printf("%*sStatement %d:\n", indent + 2, "", i + 1);
                ast_print(node->sequence.statements[i], indent + 4);
            }
            break;
    }
}
