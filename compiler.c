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
} Symbol;

typedef struct {
    Token symbol;
    int depth;
} Var;

typedef struct {
    int currentDepth;
    
    Token *params;

    Var *vars;
    int varsLength;
    int varsCapacity;
    
    Symbol symbols[100];
    int symbolsLength;
    int symbolsCapacity;

    Inst *program;
    int *programLength;
} Parser;

Parser parser;

void expression(void);
int stmt(void);
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

int var(void) {
    Token ident = pushForward();

    // 1. check variables first
    for (int i = parser.varsLength - 1; i >= 0; i--) {
        Var var = parser.vars[i];
        if (matchingTokenLexeme(var.symbol, ident) && var.depth <= parser.currentDepth) {
            pushInst((Inst){.type = INST_FETCH_VAR, .operand = createBox(&i, VAL_INT)});
            return 1;
        }
    }

    // 2. check paramaters
    for (int i = 0; parser.params + i != NULL; i++) {
        if (matchingTokenLexeme(parser.params[i], ident)) {
            pushInst((Inst){.type = INST_FETCH_ARG, .operand = createBox(&i, VAL_INT)});
            return 1;
        }
    }

    pushBack();
    return 0;
}

// primary := functionCall | var | STRING | CHARACTER | DECIMAL | INTEGER | "(" expression ")"
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
            pushInst((Inst){.type = INST_STACK_PUSH, .operand = createBox(&v, VAL_INT)});
            break;
        }
        case TOK_FLOAT: {
            double v = strtod(tr.curr.lexeme, &endptr);
            pushInst((Inst){.type = INST_STACK_PUSH, .operand = createBox(&v, VAL_FLOAT)});
            break;
        }
        case TOK_IDENT: {
            pushBack();
            if (!functionCall() && !var()) {
                Token ident = pushForward();
                fprintf(stderr, "line %d: could not find symbol or identifier for X\n", ident.line);
                exit(1);
            }
            break;
        }
        case TOK_STRING: {
            char *str = (char *)malloc(tr.curr.length + 1);
            memcpy(str, tr.curr.lexeme, tr.curr.length);
            str[tr.curr.length] = '\0';
            pushInst((Inst){.type = INST_STACK_PUSH, .operand = createBox(str, VAL_STRING)});
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

// declStmt := "let" [ "mut" ] IDENT "=" expression
int declStmt(void) {    
    if (!matchingKeyword(pushForward(), "let", 3)) {
        pushBack();
        return 0;
    }

    if (matchingKeyword(pushForward(), "mut", 3)) {
        // TODO
    } else {
        pushBack();
    }

    Token ident = pushForward(); 
    if (ident.type != TOK_IDENT) {
        fprintf(stderr, "line %d: expected identifier\n", ident.line);
        exit(1);
    }

    for (int i = parser.varsLength - 1; i >= 0; i--) {
        Var var = parser.vars[i];
        if (var.depth == parser.currentDepth && matchingTokenLexeme(var.symbol, ident)) {
            fprintf(stderr, "line %d: identifier X has already been declared on line %d\n", ident.line, var.symbol.line);
            exit(1);
        }
    }

    if (parser.varsLength >= parser.varsCapacity) {
        parser.varsCapacity *= 2;
        parser.vars = (Var *)realloc(parser.vars, sizeof(Var) * parser.varsCapacity);
    }
    parser.vars[parser.varsLength] = (Var){.depth = parser.currentDepth, .symbol = ident};
    parser.varsLength += 1; 

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
    int prevVarsLength = parser.varsLength;
    parser.currentDepth += 1;

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
            if (!stmt()) {
                fprintf(stderr, "line %d: unrecognizable statement\n", pushForward().line);
                exit(1);
            };
            goto step;
        }

    if (terminator.type != TOK_RSQUIRLY) {
        fprintf(stderr, "line %d: expected closing } for block statement\n", terminator.line);
        exit(1);
    }

    parser.currentDepth -= 1;
    int danglingOperands = parser.varsLength - prevVarsLength;
    if (danglingOperands) pushInst((Inst){.type = INST_STACK_SWEEP, .operand = createBox(&danglingOperands, VAL_INT)});
    parser.varsLength = prevVarsLength;
}

// conditionalBlock := expression programBlock
void conditionalBlock(void) {
    pushForward();
    expression();

    pushInst((Inst){.type = INST_JMP_IF_NOT});
    Inst *cjmp = &parser.program[*parser.programLength - 1];
    int jumpFrom = *parser.programLength;   

    programBlock();

    pushInst((Inst){.type = INST_SET_CB});

    // backpatch offset of programBlock
    int relativeAddr = *parser.programLength - jumpFrom + 1;
    cjmp->operand = createBox(&relativeAddr, VAL_INT);
}

// loopBlock := expression programBlock
void loopBlock(void) {
    int loopCondition = *parser.programLength;

    pushForward();
    expression();

    pushInst((Inst){.type = INST_JMP_IF_NOT});
    Inst *cjmp = &parser.program[*parser.programLength - 1];
    int jumpFrom = *parser.programLength;   

    programBlock();

    int absJmp = -(*parser.programLength - loopCondition);
    pushInst((Inst){.type = INST_JMP, .operand = createBox(&absJmp, VAL_INT)});

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
                int fallthrough = 1;
                pushInst((Inst){.type = INST_STACK_PUSH, .operand = createBox(&fallthrough, VAL_INT)});

                pushInst((Inst){.type = INST_JMP_IF_NOT});
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

// assignmentStmt := ident "=" expression
int assignmentStmt(void) {
    Token ident = pushForward();
    if (ident.type == TOK_IDENT) {
        for (int i = parser.varsLength - 1; i >= 0; i--) {
            Var var = parser.vars[i];
            if (var.depth <= parser.currentDepth && matchingTokenLexeme(var.symbol, ident)) {
                Token assignment = pushForward();
                if (assignment.type != TOK_ASSIGNMENT) {
                    fprintf(stderr, "line %d: assignment to undeclared identifier X\n", assignment.line);
                    exit(1);
                }

                pushForward();
                expression();

                pushInst((Inst){.type = INST_ASSIGN_VAR, .operand = createBox(&i, VAL_INT)});
                return 1;
            }
        }
    }
    pushBack();
    return 0;
}

int loopStmt(void) {
    Token ident = pushForward();
    if (matchingKeyword(ident, "loop", 4)) {
        loopBlock();
        return 1;
    }
    pushBack();
    return 0;
}

// stmt := functionCall; | declStmt; | returnStmt; | assignmentStmt; | ifStmt | loopStmt
int stmt(void) {
    int result = 0;

    if (declStmt() || functionCall() || returnStmt() || assignmentStmt()) {
        result = 1;
        Token semicol = pushForward();
        if (semicol.type != TOK_SEMICOL) {
            fprintf(stderr, "line %d: missing semicolon\n", semicol.line);
            exit(1);
        }
    }
    if (ifStmt() || loopStmt()) result = 1;

    return result;
}

// args := [ expression {: , expression :} ]
void args(void) {
    int numArgs = 0;

    if (pushForward().type != TOK_RPAREN) {
        numArgs += 1;

        expression();
        pushInst((Inst){.type = INST_PUSH_ARG});

        while (pushForward().type == TOK_COMMA) {
            pushForward();
            expression();
            pushInst((Inst){.type = INST_PUSH_ARG});

            numArgs += 1;
        }
    }

    pushInst((Inst){.type = INST_STACK_PUSH, .operand = createBox(&numArgs, VAL_INT)});
    pushInst((Inst){.type = INST_PUSH_ARG});
    pushBack();
}

// functionCall := IDENT "(" args ")"
int functionCall(void) {
    Token ident = pushForward();

    for (int i = 0; i < parser.symbolsLength; i++) {
        if (matchingTokenLexeme(parser.symbols[i].symbol, ident)) {
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

            pushInst((Inst){.type = INST_CALL, .operand = createBox(&parser.symbols[i].ip, VAL_INT)});
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
    if (params == NULL) {
        fprintf(stderr, "critical: unable to additional memory");
        exit(1);
    }

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
int functionDecl(void) {
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

        Symbol *symbol = parser.symbols + parser.symbolsLength;
        *symbol = (Symbol){.symbol = ident, .ip = *parser.programLength};
        parser.symbolsLength += 1;

        params(&symbol->params);
        parser.params = symbol->params;

        Token rparen = pushForward();
        if (rparen.type != TOK_RPAREN) {
            fprintf(stderr, "line %d: missing )\n", rparen.line);
            exit(1);
        }

        programBlock();

        free(symbol->params);
        parser.params = NULL;

        // TODO: PUSH AN UNDEFINED VALUE HERE
        pushInst((Inst){.type = INST_RET});

        // backpatch offset from function
        int relativeAddr = *parser.programLength - jumpFrom + 1;
        jmp->operand = createBox(&relativeAddr, VAL_INT);
        return 1;
    }
    pushBack();
    return 0;
}

// globalScope := {: stmt | function :}
void globalScope(void) {
    while (pushForward().type != TOK_EOF) {
        pushBack();
        if (!stmt() && !functionDecl()) {
            fprintf(stderr, "line %d: unrecognizable statement\n", pushForward().line);
            exit(1);
        }
    }
}

Inst* compile(int *programLength) {
    parser.programLength = programLength;
    parser.program = (Inst *)malloc(sizeof(Inst) * 4096);
    if (parser.program == NULL) {
        fprintf(stderr, "critical: unable to additional memory");
        exit(1);
    }

    parser.varsCapacity = 6;
    parser.vars = (Var *)malloc(sizeof(Var) * parser.varsCapacity);
    if (parser.vars == NULL) {
        fprintf(stderr, "critical: unable to additional memory");
        exit(1);
    }
    
    globalScope();

    free(parser.vars);
    
    return parser.program;
}
