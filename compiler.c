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
void stmt(void);

void pushInst(Inst inst) {
    parser.program[*parser.programLength] = inst;
    *parser.programLength += 1;
}

int matchingKeyword(Token token, char *keyword, size_t length) {
    if (token.length != length) return 0;
    return !memcmp(token.lexeme, keyword, length);
}

// grouping := "(" expression ")"
void grouping(void) {
    Token rparen;

    pushForward();
    expression();

    if ((rparen = pushForward()).type != TOK_RPAREN) {
        fprintf(stderr, "line %d: missing closing ) for expression\n", rparen.line);
        exit(1);
    }
}

// primary := STRING | CHARACTER | DECIMAL | INTEGER | grouping
void primary(void) {
    char *endptr;

    if (tr.curr.type == TOK_LPAREN) {
        grouping();
    } else {
        endptr = tr.curr.lexeme + tr.curr.length;

        pushInst((Inst){.type = INST_PUSH, .operand = strtol(tr.curr.lexeme, &endptr, 10)});
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
            pushInst((Inst){.type = INST_DIV});
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
            pushInst((Inst){.type = INST_SUB});
            goto step;
        default:
            pushBack();
        }
}

// comparison := term {: (">=" | ">" | "<=" | "<") term :}
void comparison(void) {
    term();

    step:
        switch (pushForward().type) {
        case TOK_GE:
            pushForward();
            term();
            pushInst((Inst){.type = INST_CMP, .operand = CMP_LT});
            goto step;
            break;
        case TOK_LE:
            pushForward();
            term();
            pushInst((Inst){.type = INST_CMP, .operand = CMP_GT});
            goto step;
            break;
        case TOK_GT:
            pushForward();
            term();
            pushInst((Inst){.type = INST_CMP, .operand = CMP_LE});
            goto step;
            break;
        case TOK_LT:
            pushForward();
            term();
            pushInst((Inst){.type = INST_CMP, .operand = CMP_GE});
            goto step;
            break;
        default:
            pushBack();
        }
}

// equality := comparison {: ("==" | "!=") comparison :}
void equality(void) {
    comparison();

    step:
        switch (pushForward().type) {
        case TOK_EQ:
            pushForward();
            comparison();
            pushInst((Inst){.type = INST_CMP, .operand = CMP_NE});
            goto step;
            break;
        case TOK_NE:
            pushForward();
            comparison();
            pushInst((Inst){.type = INST_CMP, .operand = CMP_EQ});
            goto step;
            break;
        default:
            pushBack();
        }
}

// expression := equality;
void expression(void) {
    equality();
}

// declStmt := IDENT "=" expression
void declStmt(void) {
    Token ident = pushForward(); 
    if (ident.type != TOK_IDENT) {
        fprintf(stderr, "line %d: expected identifier\n", ident.line);
        exit(1);
    }

    Token assignment = pushForward();
    if (assignment.type != TOK_ASSIGNMENT) {
        fprintf(stderr, "line %d: expected assignment after variable declaration\n", assignment.line);
        exit(1);
    }

    pushForward();
    expression();
}

// ifStmt := "(" expression ")" "{" {: stmt :} "}"
void ifStmt(void) {
    Token lparen = pushForward();
    if (lparen.type != TOK_LPAREN) {
        fprintf(stderr, "line %d: expected (\n", lparen.line);
        exit(1);
    }

    pushForward();
    expression();

    pushInst((Inst){.type = INST_CJMP});
    Inst *cjmp = &parser.program[*parser.programLength - 1];
    int jumpFrom = *parser.programLength;

    Token rparen = pushForward();
    if (rparen.type != TOK_RPAREN) {
        fprintf(stderr, "line %d: expected (\n", rparen.line);
        exit(1);
    }

    Token lsquirly = pushForward();
    if (lsquirly.type != TOK_LSQUIRLY) {
        fprintf(stderr, "line %d: expected {\n", rparen.line);
        exit(1);
    }

    Token terminator = pushForward();
    while (terminator.type != TOK_RSQUIRLY && terminator.type != TOK_EOF) {
        stmt();
        terminator = pushForward();
    }

    cjmp->operand = *parser.programLength - jumpFrom + 1;

    if (terminator.type != TOK_RSQUIRLY) {
        fprintf(stderr, "line %d: expected closing } for block statement\n", terminator.line);
        exit(1);
    }
}

// stmt := declStmt | ifStmt;
void stmt(void) {
    Token semicol;

    if (matchingKeyword(tr.curr, "var", 3)) {
        declStmt();
    } else if (matchingKeyword(tr.curr, "if", 2)) {
        ifStmt();
        return;
    } if (tr.curr.type == TOK_IDENT) {
        fprintf(stderr, "not implemented yet.");
        exit(1);
    }
    
    if ((semicol = pushForward()).type != TOK_SEMICOL) {
        fprintf(stderr, "line %d: missing semicolon\n", semicol.line);
        exit(1);
    }
}

Inst* compile(int *programLength) {
    parser.programLength = programLength;
    parser.program = (Inst *)malloc(sizeof(Inst) * 4096);

    Token terminator = pushForward();
    while (terminator.type != TOK_EOF) {
        stmt();
        terminator = pushForward();
    }

    return parser.program;
}
