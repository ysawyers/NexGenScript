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
    Token symbol;
    int ip;
} Label;

typedef struct {
    Label labels[100];
    int labelsSize;
    Inst *program;
    int *programLength;
} Parser;

Parser parser;

void expression(void);
void stmt(void);
int functionCall(void);

void pushInst(Inst inst) {
    parser.program[*parser.programLength] = inst;
    *parser.programLength += 1;
}

int matchingKeyword(Token token, char *keyword, size_t length) {
    if (token.length != length) return 0;
    return !memcmp(token.lexeme, keyword, length);
}

int matchingTokenLexeme(Token a, Token b) {
    if (a.length != b.length) return 0;
    return !memcmp(a.lexeme, b.lexeme, a.length);
}

// primary := functionCall | STRING | CHARACTER | DECIMAL | INTEGER | "(" expression ")"
void primary(void) {
    if (tr.curr.type == TOK_LPAREN) {
        pushForward();
        expression();

        Token rparen = pushForward();
        if (rparen.type != TOK_RPAREN) {
            fprintf(stderr, "line %d: missing closing ) for expression\n", rparen.line);
            exit(1);
        }
    } else {
        char *endptr = tr.curr.lexeme + tr.curr.length;

        switch (tr.curr.type) {
        case TOK_INTEGER: {
            long v = strtol(tr.curr.lexeme, &endptr, 10);
            pushInst((Inst){.type = INST_PUSH, .operand = createBox(&v, VAL_INT)});
            break;
        }
        case TOK_FLOAT: {
            double v = strtod(tr.curr.lexeme, &endptr);
            pushInst((Inst){.type = INST_PUSH, .operand = createBox(&v, VAL_FLOAT)});
            break;
        }
        case TOK_IDENT: {
            pushBack();
            if (!functionCall()) {
                fprintf(stderr, "have not implemented variables yet!");
                exit(1);
            }
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
            c = CMP_GE;
            pushForward();
            term();
            pushInst((Inst){.type = INST_CMP, .operand = createBox(&c, VAL_INT)});
            goto step;
            break;
        case TOK_LE:
            c = CMP_LE;
            pushForward();
            term();
            pushInst((Inst){.type = INST_CMP, .operand = createBox(&c, VAL_INT)});
            goto step;
            break;
        case TOK_GT:
            c = CMP_GT;
            pushForward();
            term();
            pushInst((Inst){.type = INST_CMP, .operand = createBox(&c, VAL_INT)});
            goto step;
            break;
        case TOK_LT:
            c = CMP_LT;
            pushForward();
            term();
            pushInst((Inst){.type = INST_CMP, .operand = createBox(&c, VAL_INT)});
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
            c = CMP_EQ;
            pushForward();
            comparison();
            pushInst((Inst){.type = INST_CMP, .operand = createBox(&c, VAL_INT)});
            goto step;
            break;
        case TOK_NE:
            c = CMP_NE;
            pushForward();
            comparison();
            pushInst((Inst){.type = INST_CMP, .operand = createBox(&c, VAL_INT)});
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

// programBlock := "{" {: stmt [ "return" expression; (then BREAK) ] :} "}"
int programBlock(void) {
    Token lsquirly = pushForward();
    if (lsquirly.type != TOK_LSQUIRLY) {
        fprintf(stderr, "line %d: expected {\n", lsquirly.line);
        exit(1);
    }

    Token terminator;

    // lets external context know that this block returns a value and should preserve the top value on the stack after cleanup
    int isReturningValue = 0;

    step:
        terminator = pushForward();

        if (matchingKeyword(terminator, "return", 6)) {
            pushForward();
            expression();

            Token semicol = pushForward();
            if (semicol.type != TOK_SEMICOL) {
                fprintf(stderr, "line %d: missing semicol\n", semicol.line);
                exit(1);
            }

            isReturningValue = 1;
            terminator = pushForward();
        } else {
            switch(terminator.type) {
            case TOK_RSQUIRLY:
            case TOK_EOF:
                break;
            default:
                pushBack();
                stmt();
                goto step;
            }
        }

    if (terminator.type != TOK_RSQUIRLY) {
        fprintf(stderr, "line %d: expected closing } for block statement\n", terminator.line);
        exit(1);
    }

    return isReturningValue;
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

    // logical NOT condition to check if a jump (skip) is necessary (will be true if condition was false)
    pushInst((Inst){.type = INST_NOT});

    pushInst((Inst){.type = INST_CJMP});
    Inst *cjmp = &parser.program[*parser.programLength - 1];
    int jumpFrom = *parser.programLength;    

    Token rparen = pushForward();
    if (rparen.type != TOK_RPAREN) {
        fprintf(stderr, "line %d: expected )\n", rparen.line);
        exit(1);
    }

    programBlock();

    pushInst((Inst){.type = INST_SET_CB});

    // backpatch offset of programBlock
    int relativeAddr = *parser.programLength - jumpFrom + 1;
    cjmp->operand = createBox(&relativeAddr, VAL_INT);
}

// ifStmt := "if" conditionalBlock [ "else" (ifStmt | programBlock) ]
int ifStmt(void) {
    if (matchingKeyword(pushForward(), "if", 2)) {
        conditionalBlock();
        if (matchingKeyword(pushForward(), "else", 4)) {
            if (!ifStmt()) {
                int fallback = 0;
                pushInst((Inst){.type = INST_PUSH, .operand = createBox(&fallback, VAL_INT)});

                pushInst((Inst){.type = INST_CJMP});
                Inst *cjmp = &parser.program[*parser.programLength - 1];
                int jumpFrom = *parser.programLength;

                programBlock();

                // backpatch offset of programBlock
                int relativeAddr = *parser.programLength - jumpFrom + 1;
                cjmp->operand = createBox(&relativeAddr, VAL_INT);
            };
        } else {
            pushBack();
        }

        pushInst((Inst){.type = INST_UNSET_CB});
        return 1;
    }
    pushBack();
    return 0;
}

// stmt := functionCall; | declStmt; | ifStmt
void stmt(void) {
    if (declStmt() || functionCall()) {
        Token semicol = pushForward();
        if (semicol.type != TOK_SEMICOL) {
            fprintf(stderr, "line %d: missing semicolon\n", semicol.line);
            exit(1);
        }
    }
    ifStmt();
}

// args := [ expression {: , expression :} ]
void args(void) {
    if (pushForward().type != TOK_RPAREN) {
        expression();

        while (pushForward().type == TOK_COMMA) {
            pushForward();
            expression();
        }
        pushBack();
    } else {
        pushBack();
    }
}

// functionCall := IDENT "(" args ")"
int functionCall(void) {
    Token ident = pushForward();

    for (int i = 0; i < parser.labelsSize; i++) {
        if (matchingTokenLexeme(parser.labels[i].symbol, ident)) {
            Token lparen = pushForward();
            if (lparen.type != TOK_LPAREN) {
                fprintf(stderr, "line %d: expected (\n", lparen.line);
                exit(1);
            }
            
            args();

            Token rparen = pushForward();
            if (rparen.type != TOK_RPAREN) {
                fprintf(stderr, "line %d: expected )\n", rparen.line);
                exit(1);
            }

            pushInst((Inst){.type = INST_CALL, .operand = createBox(&parser.labels[i].ip, VAL_INT)});
            return 1;
        }
    }

    pushBack();
    return 0;
}

// params := [ IDENT {: , IDENT :} ]
void params(void) {
    Token param = pushForward();
    if (param.type != TOK_IDENT) {
        pushBack();
        return;
    };

    while (pushForward().type == TOK_COMMA) {
        param = pushForward();
        if (param.type != TOK_IDENT) {
            fprintf(stderr, "line %d: expected identifier\n", param.line);
            exit(1);
        }
    }
    pushBack();
}

// functionDecl := "fun" IDENT "(" params ")" programBlock
void functionDecl(void) {
    if (matchingKeyword(pushForward(), "fun", 3)) {
        Token ident = pushForward();
        if (ident.type != TOK_IDENT) {
            fprintf(stderr, "line %d: expected identifier for function declaration\n", ident.line);
            exit(1);
        }

        // force program to skip over the instructions specified from function definition and will only be hit on CALL instruction
        int skipOverDefinition = 1;
        pushInst((Inst){.type = INST_PUSH, .operand = createBox(&skipOverDefinition, VAL_INT)});

        pushInst((Inst){.type = INST_CJMP});
        Inst *cjmp = &parser.program[*parser.programLength - 1];
        int jumpFrom = *parser.programLength;

        parser.labels[parser.labelsSize] = (Label){.symbol = ident, .ip = *parser.programLength};
        parser.labelsSize += 1;

        Token lparen = pushForward();
        if (lparen.type != TOK_LPAREN) {
            fprintf(stderr, "line %d: missing (\n", lparen.line);
            exit(1);
        }

        params();

        Token rparen = pushForward();
        if (rparen.type != TOK_RPAREN) {
            fprintf(stderr, "line %d: missing )\n", rparen.line);
            exit(1);
        }

        int isReturningValue = programBlock();
        pushInst((Inst){.type = INST_RET, .operand = createBox(&isReturningValue, VAL_INT)});

        // backpatch offset from function
        int relativeAddr = *parser.programLength - jumpFrom + 1;
        cjmp->operand = createBox(&relativeAddr, VAL_INT);
    } else {
        pushBack();
    }
}

// global := {: stmt | function :}
void global(void) {
    while (pushForward().type != TOK_EOF) {
        pushBack();
        stmt();
        functionDecl();
    }
}

Inst* compile(int *programLength) {
    parser.programLength = programLength;
    parser.program = (Inst *)malloc(sizeof(Inst) * 4096);
    parser.labelsSize = 0;
    global();
    return parser.program;
}
