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
    INST_PUSH,
    
    // OPERATIONS
    INST_ADD,
    INST_SUB,
    INST_MULT,
    INST_DIV,
    INST_LOGICAL_NOT,
    INST_CMP,

    // LOOP (IMPLEMENT WHAT THE HECK WE ALREADY HAVE A USE CASE FOR THIS!!)
    INST_JMP,
    
    // CONDITIONALS
    INST_CJMP,
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
    Box stack[MEM_SIZE];
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

void programDump(void);

void memoryDump(void);

void dumpCallStack(void);

void printBox(Box box);

#endif
