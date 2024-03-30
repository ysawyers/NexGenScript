#ifndef SCANNER_H
#define SCANNER_H

typedef enum {
    TOK_ASSIGNMENT,

    // LITERALS
    TOK_INTEGER,
    TOK_FLOAT,
    TOK_STRING,

    // BRACKETS
    TOK_LPAREN, 
    TOK_RPAREN,
    TOK_LSQUIRLY,
    TOK_RSQUIRLY,

    // DELIMETERS
    TOK_SEMICOL,
    TOK_COMMA,

    // OPERATORS
    TOK_PLUS,
    TOK_MINUS,
    TOK_MULT,
    TOK_DIV,
    TOK_EQ,
    TOK_NE,
    TOK_GT,
    TOK_GE,
    TOK_LE,
    TOK_LT,

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

void scannerInitialize(char *file);

void printToken(Token token);

void debugScanner(void);

#endif
