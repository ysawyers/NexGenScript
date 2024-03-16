#ifndef SCANNER_H
#define SCANNER_H

typedef enum {
    TOK_ASSIGNMENT,

    // LITERALS
    TOK_INTEGER,
    TOK_DECIMAL,

    // BRACKETS
    TOK_LPAREN, 
    TOK_RPAREN,

    // DELIMETERS
    TOK_SEMICOL,

    // OPERATORS
    TOK_PLUS,
    TOK_MINUS,
    TOK_MULT,
    TOK_DIV,

    // SYMBOLS
    TOK_KEYWORD,
    TOK_IDENT,

    // MISC
    TOK_EOF,
    TOK_ERR,
} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;
    unsigned int length;
    int line;
} Token;

Token nextToken(void);

void scannerInitialize(char *buffer);

void printToken(Token token);

void debugScanner(void);

#endif
