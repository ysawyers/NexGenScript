#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "scanner.h"

typedef struct {
    char *start;
    char *current;
    int line;
} Scanner;

Scanner scanner;

void scannerInitialize(char *buffer) {
    scanner.start = buffer;
    scanner.current = buffer;
    scanner.line = 1;
}

static Token constructToken(TokenType type) {
    Token token;

    token.type = type;
    token.length = scanner.current - scanner.start;
    token.lexeme = scanner.start;
    token.line = scanner.line;
    
    return token;
}

static int compareKeyword(char *keyword, unsigned long length) {
    return !memcmp(keyword, scanner.start, length);
}

static Token isSymbolOrKeyword(void) {
    while (isalnum(*scanner.current)) {
        scanner.current++;
    }

    int matchedKeyword = 0;

    matchedKeyword = compareKeyword("var", 3);

    return constructToken(matchedKeyword ? TOK_KEYWORD : TOK_IDENT);
}

static Token isNumber(void) {
    int isDecimal;

    isDecimal = 0;

    while (isdigit(*scanner.current) || *scanner.current == '.') {
        if (*scanner.current == '.') {
            isDecimal = 1;
        }
        scanner.current++;
    }

    if (*(scanner.current - 1) == '.') {
        return constructToken(TOK_ERR);
    }

    return constructToken(isDecimal ? TOK_DECIMAL : TOK_INTEGER);
}

Token nextToken(void) {
    Token tok;
    char c;

    if (*scanner.start == '\0') return constructToken(TOK_EOF);
    tok.line = -1;

    while (1) 
    {
        c = *(scanner.current++);

        if (isalpha(c)) {
            tok = isSymbolOrKeyword();
        } else if (isdigit(c)) {
            tok = isNumber();
        } else {
            switch (c) {
            case ';':
                tok = constructToken(TOK_SEMICOL);
                break;
            case '=':
                tok = constructToken(TOK_ASSIGNMENT);
                break;
            case ')':
                tok = constructToken(TOK_RPAREN);
                break;
            case '(':
                tok = constructToken(TOK_LPAREN);
                break;
            case '+':
                tok = constructToken(TOK_PLUS);
                break;
            case '-':
                tok = constructToken(TOK_MINUS);
                break;
            case '/':
                tok = constructToken(TOK_DIV);
                break;
            case '*':
                tok = constructToken(TOK_MULT);
                break;
            case '\n':
                scanner.line++;
                break;

            // SPECIAL CASE: current will always be ahead of start so when start is pointing to EOF current will be pointing to garbage
            case '\0':
                return constructToken(TOK_EOF);
            }
        }

        scanner.start = scanner.current;

        if (tok.line != -1) {
            return tok;
        }
    }
}

const char* stringifyToken(Token token) {
    switch (token.type) {
    case TOK_SEMICOL: return "TOK_SEMICOL";
    case TOK_ASSIGNMENT: return "TOK_ASSIGNMENT";
    case TOK_INTEGER: return "TOK_INTEGER"; 
    case TOK_DECIMAL: return "TOK_DECIMAL";
    case TOK_LPAREN: return "TOK_LPAREN";
    case TOK_RPAREN: return "TOK_RPAREN";
    case TOK_PLUS: return "TOK_PLUS";
    case TOK_MINUS: return "TOK_MINUS";
    case TOK_MULT: return "TOK_MULT";
    case TOK_DIV: return "TOK_DIV";
    case TOK_KEYWORD: return "TOK_KEYWORD";
    case TOK_IDENT: return "TOK_IDENT";
    case TOK_EOF: return "TOK_EOF";
    case TOK_ERR: return "TOK_ERR";
    }
}

void printToken(Token token) {
    printf("%s ", stringifyToken(token));
    for (unsigned int i = 0; i < token.length; i++) {
        printf("%c", token.lexeme[i]);
    }
    printf("\n");
}

void debugScanner(void) {
    Token tok;

    while ((tok = nextToken()).type != TOK_EOF) {
        printToken(tok);
    }
}
