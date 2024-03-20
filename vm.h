#ifndef VM_H
#define VM_H

#include <stdlib.h>
#include "value.h"

#define MEM_SIZE 4096

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
    INST_NOT,
    INST_JMP,
    INST_CJMP,
    INST_RESET_CB,
} InstType;

typedef struct {
    InstType type;
    Box operand;
} Inst;

typedef struct {
    Box stack[MEM_SIZE];
    int sp;

    Inst *program;
    int pc;
    int programLength;

    // if a conditional statement passes (CJMP returns 0) set this to 1 to force all other chained conditions to fallthrough
    int conditionBreaker;
} VM;

void initVM(Inst *instructions, size_t length);
void freeVM(void);

void executeProgram(void);

void programDump(void);

void memoryDump(void);

#endif
