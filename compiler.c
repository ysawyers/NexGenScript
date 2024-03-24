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
    Token *params;
    int ip;
} Label;

typedef struct {
    Token *params; // something or NULL
} Scope;

typedef struct {
    Label labels[100];
    int labelsLength;

    Scope currentScope;
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

// primary := functionCall | STRING | CHARACTER | DECIMAL | INTEGER | "(" expression ")" | IDENT
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
                Token ident = pushForward();

                // 1. check local scope

                // 2. check paramaters (only applies inside function decl)
                for (int i = 0; parser.currentScope.params + i != NULL; i++) {
                    if (matchingTokenLexeme(parser.currentScope.params[i], ident)) {
                        pushInst((Inst){.type = INST_FETCH_ARG, .operand = createBox(&i, VAL_INT)});
                        break;
                    }
                }

                // 3. check global scope
            }
            break;
        }
        default:
            fprintf(stderr, "line %d: expected identifier or expression\n", tr.curr.line);
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
void programBlock(void) {
    Token lsquirly = pushForward();
    if (lsquirly.type != TOK_LSQUIRLY) {
        fprintf(stderr, "line %d: expected {\n", lsquirly.line);
        exit(1);
    }

    Token terminator;

    step:
        terminator = pushForward();

        switch(terminator.type) {
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

    // logical NOT condition to check if a jump (skip) is necessary (will be true if condition was false)
    pushInst((Inst){.type = INST_LOGICAL_NOT});

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

int returnStmt(void) {
    if (matchingKeyword(pushForward(), "return", 6)) {
        pushForward();
        expression();
        pushInst((Inst){.type = INST_RET});
        return 1;
    }
    pushBack();
    return 0;
}

// stmt := functionCall; | declStmt; | returnStmt; | ifStmt
void stmt(void) {
    if (declStmt() || functionCall() || returnStmt()) {
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
        int numArgs = 1;

        expression();
        pushInst((Inst){.type = INST_PUSH_ARG});

        while (pushForward().type == TOK_COMMA) {
            pushForward();
            expression();
            pushInst((Inst){.type = INST_PUSH_ARG});

            numArgs += 1;
        }

        pushInst((Inst){.type = INST_PUSH, .operand = createBox(&numArgs, VAL_INT)});
        pushInst((Inst){.type = INST_PUSH_ARG});
    }
    pushBack();
}

// functionCall := IDENT "(" args ")"
int functionCall(void) {
    Token ident = pushForward();

    for (int i = 0; i < parser.labelsLength; i++) {
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
void params(Token **params) {
    Token param = pushForward();
    if (param.type != TOK_IDENT) {
        pushBack();
        return;
    };

    int paramsLength = 6;
    *params = (Token *)malloc(sizeof(Token) * paramsLength);

    int i = 0;
    (*params)[i++] = param;

    while (pushForward().type == TOK_COMMA) {
        param = pushForward();
        if (param.type != TOK_IDENT) {
            fprintf(stderr, "line %d: expected identifier\n", param.line);
            exit(1);
        }

        if (i >= paramsLength) {
            *params = realloc(*params, paramsLength * 2);
            paramsLength = paramsLength * 2;
        }

        (*params)[i++] = param;
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

        pushInst((Inst){.type = INST_JMP});
        Inst *jmp = &parser.program[*parser.programLength - 1];
        int jumpFrom = *parser.programLength;

        Token lparen = pushForward();
        if (lparen.type != TOK_LPAREN) {
            fprintf(stderr, "line %d: missing (\n", lparen.line);
            exit(1);
        }

        Label *label = parser.labels + parser.labelsLength;
        *label = (Label){.symbol = ident, .ip = *parser.programLength};
        parser.labelsLength += 1;

        params(&label->params);
        parser.currentScope.params = label->params;

        Token rparen = pushForward();
        if (rparen.type != TOK_RPAREN) {
            fprintf(stderr, "line %d: missing )\n", rparen.line);
            exit(1);
        }

        programBlock();

        free(label->params);
        parser.currentScope.params = NULL;

        // TODO: PUSH AN UNDEFINED VALUE HERE
        pushInst((Inst){.type = INST_RET});

        // backpatch offset from function
        int relativeAddr = *parser.programLength - jumpFrom + 1;
        jmp->operand = createBox(&relativeAddr, VAL_INT);
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
    parser.labelsLength = 0;
    global();
    return parser.program;
}
