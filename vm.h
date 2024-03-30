#ifndef VM_H
#define VM_H

#include <stdlib.h>
#include "value.h"

#define MEM_SIZE 4096
#define CALLSTACK_MAX_SIZE 9 * 3200

typedef enum {
    CMP_GT,
    CMP_LT,
    CMP_GE,
    CMP_LE,
    CMP_EQ,
    CMP_NE,
} Condition;

typedef enum {
    // STACK
    INST_STACK_PUSH,
    INST_STACK_SWEEP,
    INST_FETCH_VAR,
    INST_ASSIGN_VAR,

    // OPERATIONS
    INST_ADD,
    INST_SUB,
    INST_MULT,
    INST_DIV,
    INST_CMP,
    INST_LOGICAL_NOT,

    // JUMPS
    INST_JMP,
    INST_JMP_IF_NOT,

    // CONDITIONALS
    INST_SET_CB,
    INST_UNSET_CB,
    
    // FUNCTIONS
    INST_CALL,
    INST_PUSH_ARG,
    INST_FETCH_ARG,
    INST_RET,
} InstType;

typedef struct {
    InstType type;
    Box operand;
} Inst;

typedef struct {
    size_t count;
    void *ref;
} Reference;

typedef struct {
    Box operandStack[MEM_SIZE];
    int sp;
    
    Box callStack[CALLSTACK_MAX_SIZE];
    int csp;

    Inst *program;
    int pc;
    int programLength;

    // if a conditional statement passes (CJMP returns 0) set this to 1 to force all other chained conditions to fallthrough
    int conditionBreaker;
} VM;

void initVM(Inst *instructions, size_t length);
void freeVM(void);

void executeProgram(void);

void dumpProgram(void);
void dumpOperandStack(void);
void dumpCallStack(void);

char* stringifyInst(InstType type);
void printBox(Box box);

#endif
