#ifndef VM_H
#define VM_H

#include <stdlib.h>

#define MEM_SIZE 4096

typedef int64_t Word;

typedef enum {
    CMP_GT,
    CMP_LT,
    CMP_GE,
    CMP_LE,
    CMP_EQ,
    CMP_NE,
} Conditions;

typedef enum {
    INST_PUSH,
    INST_ADD,
    INST_SUB,
    INST_MULT,
    INST_DIV,
    INST_CMP,    
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

VM* initVM(Inst *instructions, size_t length);
void freeVM(VM *vm);

void executeProgram(VM *vm);

void programDump(VM *vm);

void memoryDump(VM *vm);

#endif
