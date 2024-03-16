#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"

typedef struct {
    Token prev;
    Token curr;
    int steppedBack;
} TokenReader;

TokenReader tr;

Token pushForward(void) {
    if (tr.steppedBack) {
        tr.steppedBack = 0;
    } else {
        tr.prev = tr.curr;
        tr.curr = nextToken();
    }
    return tr.curr;
}

void pushBack(void) {
    tr.steppedBack = 1;
}

typedef struct {
    Inst *program;
    int *programLength;
} Parser;

Parser parser;

void expression(void);

void pushInst(Inst inst) {
    parser.program[*parser.programLength] = inst;
    *parser.programLength += 1;
}

int matchingKeyword(Token token, char *keyword, size_t length) {
    if (token.length != length) return 0;
    return !memcmp(token.lexeme, keyword, length);
}

// primary := STRING | CHARACTER | DECIMAL | INTEGER | "(" expression ")"
void primary(void) {
    if (tr.curr.type == TOK_LPAREN) {
        pushForward();
        expression();
    } else {
        pushInst((Inst){.type = INST_PUSH, .operand = 10});
    }
}

// unary := ("+" | "-") unary | primary
void unary(void) {
    primary();
}

// factor := unary {: ("*" | "/") unary :}
void factor(void) {
    unary();

    step:
        switch (pushForward().type) {
        case TOK_MULT:
            pushForward();
            unary();
            pushInst((Inst){.type = INST_MULT});
            goto step;
        case TOK_DIV:
            pushForward();
            unary();
            pushInst((Inst){.type = INST_MULT});
            goto step;
        default:
            pushBack();
        }
}

// term := factor {: ("+" | "-") factor :}
void term(void) {
    factor();

    step:
        switch (pushForward().type) {
        case TOK_PLUS:
            pushForward();
            factor();
            pushInst((Inst){.type = INST_ADD});
            goto step;
        case TOK_MINUS:
            pushForward();
            factor();
            pushInst((Inst){.type = INST_ADD});
            goto step;
        default:
            pushBack();
        }
}

// comparison := term {: (">=" | ">" | "<=" | "<") term :}
void comparison(void) {
    term();
}

// equality := comparison {: ("==" | "!=") comparison :}
void equality(void) {
    comparison();
}

// expression := equality;
void expression(void) {
    equality();
}

// declStmt := ("var" | "const") IDENT "=" expression
void declStmt(void) {
    Token declType;
    Token ident;
    Token assignment;

    declType = pushForward();
    if (matchingKeyword(declType, "var", 3) || matchingKeyword(declType, "const", 3)) {
        // TODO: FIX THESE TOKENS IN THE LEXER
    }

    ident = pushForward();
    if (ident.type != TOK_IDENT) {
        fprintf(stderr, "line %d: expected identifier", ident.line);
        exit(1);
    }

    assignment = pushForward();
    if (assignment.type != TOK_ASSIGNMENT) {
        fprintf(stderr, "line %d: expected assignment after variable declaration", assignment.line);
        exit(1);
    }

    pushForward();
    expression();
}

// stmt := declStmt;
void stmt(void) {
    Token stmtType;
    Token semicol;

    stmtType = pushForward();

    if (matchingKeyword(stmtType, "var", 3)) {
        pushBack();
        declStmt();
    } else if (stmtType.type == TOK_IDENT) {
        fprintf(stderr, "not implemented yet.");
        exit(1);
    }
    
    semicol = pushForward();
    if (semicol.type != TOK_SEMICOL) {
        fprintf(stderr, "line %d: missing semicolon\n", semicol.line);
        exit(1);
    }
}

Inst* compile(int *programLength) {
    parser.programLength = programLength;
    parser.program = (Inst *)malloc(sizeof(Inst) * 4096);

    while (tr.curr.type != TOK_EOF) {
        stmt();
        pushForward();
    }

    return parser.program;
}
