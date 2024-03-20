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
    if (tr.curr.type == TOK_LPAREN) {
        grouping();
    } else {
        char *endptr = tr.curr.lexeme + tr.curr.length;

        switch (tr.curr.type) {
        case TOK_INTEGER: {
            long v = strtol(tr.curr.lexeme, &endptr, 10);
            pushInst((Inst){.type = INST_PUSH, .operand = encodeTaggedLiteral(&v, VAL_INT)});
            break;
        }
        case TOK_FLOAT: {
            double v = strtod(tr.curr.lexeme, &endptr);
            pushInst((Inst){.type = INST_PUSH, .operand = encodeTaggedLiteral(&v, VAL_FLOAT)});
            break;
        } 
        default:
            fprintf(stderr, "line %d: expected identifier or expression", tr.curr.line);
            exit(1);
        }
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

    Condition c;

    step:
        switch (pushForward().type) {
        case TOK_GE:
            c = CMP_LT;
            pushForward();
            term();
            pushInst((Inst){.type = INST_CMP, .operand = encodeTaggedLiteral(&c, VAL_INT)});
            goto step;
            break;
        case TOK_LE:
            c = CMP_GT;
            pushForward();
            term();
            pushInst((Inst){.type = INST_CMP, .operand = encodeTaggedLiteral(&c, VAL_INT)});
            goto step;
            break;
        case TOK_GT:
            c = CMP_LE;
            pushForward();
            term();
            pushInst((Inst){.type = INST_CMP, .operand = encodeTaggedLiteral(&c, VAL_INT)});
            goto step;
            break;
        case TOK_LT:
            c = CMP_GE;
            pushForward();
            term();
            pushInst((Inst){.type = INST_CMP, .operand = encodeTaggedLiteral(&c, VAL_INT)});
            goto step;
            break;
        default:
            pushBack();
        }
}

// equality := comparison {: ("==" | "!=") comparison :}
void equality(void) {
    comparison();

    Condition c;

    step:
        switch (pushForward().type) {
        case TOK_EQ:
            c = CMP_NE;
            pushForward();
            comparison();
            pushInst((Inst){.type = INST_CMP, .operand = encodeTaggedLiteral(&c, VAL_INT)});
            goto step;
            break;
        case TOK_NE:
            c = CMP_EQ;
            pushForward();
            comparison();
            pushInst((Inst){.type = INST_CMP, .operand = encodeTaggedLiteral(&c, VAL_INT)});
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

// declStmt := ("var" | "const") IDENT "=" expression
int declStmt(void) {
    Token declType = pushForward();
    if (matchingKeyword(declType, "var", 3)) {
        // TODO
    } else if (matchingKeyword(declType, "const", 5)) {
        // TODO
    } else {
        pushBack();
        return 0;
    }

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

    return 1;
}

// programBlock := "{" {: stmt :} "}"
void programBlock(void) {
    Token lsquirly = pushForward();
    if (lsquirly.type != TOK_LSQUIRLY) {
        fprintf(stderr, "line %d: expected {\n", lsquirly.line);
        exit(1);
    }

    Token terminator;

    step:
        switch((terminator = pushForward()).type) {
        case TOK_RSQUIRLY:
        case TOK_EOF:
            break;
        default:
            pushBack();
            stmt();
            goto step;
        }

    if (terminator.type != TOK_RSQUIRLY) {
        fprintf(stderr, "line %d: expected closing } for block statement\n", terminator.line);
        exit(1);
    }
}

// conditionalBlock := "(" expression ")" programBlock
void conditionalBlock(void) {
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
        fprintf(stderr, "line %d: expected )\n", rparen.line);
        exit(1);
    }

    programBlock();

    int relativeAddr = *parser.programLength - jumpFrom + 1;
    cjmp->operand = encodeTaggedLiteral(&relativeAddr, VAL_INT);
}

// ifStmt := "if" conditionalBlock [ "else" (ifStmt | programBlock) ]
int ifStmt(void) {
    if (matchingKeyword(pushForward(), "if", 2)) {
        conditionalBlock();
        if (matchingKeyword(pushForward(), "else", 4)) {
            if (!ifStmt()) programBlock();
        } else {
            pushBack();
        }
        return 1;
    }
    pushBack();
    return 0;
}

// stmt := declStmt; | ifStmt
void stmt(void) {
    if (declStmt()) {
        Token semicol = pushForward();
        if (semicol.type != TOK_SEMICOL) {
            fprintf(stderr, "line %d: missing semicolon\n", semicol.line);
            exit(1);
        }
    }

    // does not require ;
    ifStmt();
}

Inst* compile(int *programLength) {
    parser.programLength = programLength;
    parser.program = (Inst *)malloc(sizeof(Inst) * 4096);

    while (pushForward().type != TOK_EOF) {
        pushBack();
        stmt();
    }

    return parser.program;
}
