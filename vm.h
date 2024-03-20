#ifndef VM_H
#define VM_H

#include <stdlib.h>
#include "value.h"

#define MEM_SIZE 4096

typedef double Word;

typedef enum {
    CMP_GT,
    CMP_LT,
    CMP_GE,
    CMP_LE,
    CMP_EQ,
    CMP_NE,
} Condition;

typedef enum {
    INST_PUSH,
    INST_ADD,
    INST_SUB,
    INST_MULT,
    INST_DIV,
    INST_CMP,
    INST_JMP,
    INST_CJMP,    
} InstType;

typedef struct {
    InstType type;
    Word operand;
} Inst;

typedef struct {
    Word stack[MEM_SIZE];
    int sp;

    Inst *program;
    int pc;
    int programLength;
} VM;

void initVM(Inst *instructions, size_t length);
void freeVM(void);

void executeProgram(void);

void programDump(void);

void memoryDump(void);

double encodeTaggedLiteral(void *value, ValueType type);

#endif
