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

void scannerInitialize(char *file) {
    scanner.start = file;
    scanner.current = file;
    scanner.line = 1;
}

Token constructToken(TokenType type) {
    Token token;

    token.type = type;
    token.length = scanner.current - scanner.start;
    token.lexeme = scanner.start;
    token.line = scanner.line;
    
    return token;
}

int compareKeyword(char *keyword, long length) {
    if ((scanner.current - scanner.start) != length) return 0;
    return !memcmp(keyword, scanner.start, length);
}

Token isSymbolOrKeyword(void) {
    while (isalnum(*scanner.current)) {
        scanner.current++;
    }

    int matchedKeyword = 0;

    matchedKeyword = compareKeyword("fun", 3);
    matchedKeyword = compareKeyword("let", 3);
    matchedKeyword = compareKeyword("if", 2);
    matchedKeyword = compareKeyword("else", 4);
    matchedKeyword = compareKeyword("return", 6);
    matchedKeyword = compareKeyword("loop", 4);

    return constructToken(matchedKeyword ? TOK_KEYWORD : TOK_IDENT);
}

Token isNumber(void) {
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

    return constructToken(isDecimal ? TOK_FLOAT : TOK_INTEGER);
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
            case '"':
                scanner.start++;
                while (*scanner.current != '"' && *scanner.current != '\0') scanner.current++;
                if (*scanner.current == '\0') {
                    fprintf(stderr, "you messed up the string bruh");
                    exit(1);
                }
                tok = constructToken(TOK_STRING);
                scanner.current++;
                break;
            case ';':
                tok = constructToken(TOK_SEMICOL);
                break;
            case '=':
                if (*scanner.current == '=') {
                    tok = constructToken(TOK_EQ);
                    scanner.current += 1;
                } else {
                    tok = constructToken(TOK_ASSIGNMENT);
                }
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
                if (*scanner.current == '/') {
                    while (*scanner.current != '\n') scanner.current++;
                    scanner.current++;
                } else {
                    tok = constructToken(TOK_DIV);
                }
                break;
            case '*':
                tok = constructToken(TOK_MULT);
                break;
            case ',':
                tok = constructToken(TOK_COMMA);
                break;
            case '{':
                tok = constructToken(TOK_LSQUIRLY);
                break;
            case '}':
                tok = constructToken(TOK_RSQUIRLY);
                break;
            case '>':
                if (*scanner.current == '=') {
                    tok = constructToken(TOK_GE);
                    scanner.current += 1;
                } else {
                    tok = constructToken(TOK_GT);
                }
                break;
            case '<':
                if (*scanner.current == '=') {
                    tok = constructToken(TOK_LE);
                    scanner.current += 1;
                } else {
                    tok = constructToken(TOK_LT);
                }
                break;
            case '!':
                if (*scanner.current == '=') {
                    tok = constructToken(TOK_NE);
                    scanner.current += 1;
                }
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

char* stringifyToken(Token token) {
    switch (token.type) {
    case TOK_STRING: return "TOK_STRING";
    case TOK_COMMA: return "TOK_COMMA";
    case TOK_LSQUIRLY: return "TOK_LSQUIRLY";
    case TOK_RSQUIRLY: return "TOK_RSQUIRLY";
    case TOK_EQ: return "TOK_EQ";
    case TOK_NE: return "TOK_NE";
    case TOK_GT: return "TOK_GT";
    case TOK_LT: return "TOK_LT";
    case TOK_GE: return "TOK_GE";
    case TOK_LE: return "TOK_LE";
    case TOK_SEMICOL: return "TOK_SEMICOL";
    case TOK_ASSIGNMENT: return "TOK_ASSIGNMENT";
    case TOK_INTEGER: return "TOK_INTEGER"; 
    case TOK_FLOAT: return "TOK_FLOAT";
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
