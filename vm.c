#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vm.h"

VM *vm;

void initVM(Inst *program, size_t length) {
    vm = malloc(sizeof(VM));
    vm->sp = -1;
    vm->programLength = length;
    vm->program = program;
    vm->conditionBreaker = 0;
}

void freeVM(void) {
    free(vm->program);
    free(vm);
}

static inline void push(Box value) {
    if (vm->sp + 1 >= MEM_SIZE) {
        fprintf(stderr, "VM ERROR: stack overflow\n");
        exit(1);
    }

    vm->sp += 1;
    vm->stack[vm->sp] = value;
}

static inline Box pop(void) {
    if (vm->sp < 0) {
        fprintf(stderr, "VM ERROR: stack underflow\n");
        exit(1);
    }

    Box value = vm->stack[vm->sp];
    vm->sp -= 1;
    return value;
}

static inline Box read(int addr) {
    if (addr > vm->sp || addr >= MEM_SIZE || addr < 0) {
        fprintf(stderr, "VM ERROR: invalid READ access at location %d\n", addr);
        exit(1);
    }

    return vm->stack[addr];
}

static inline void write(int addr, Box value) {
    if (addr > vm->sp || addr >= MEM_SIZE || addr < 0) {
        fprintf(stderr, "VM ERROR: invalid WRITE access at location %d\n", addr);
        exit(1);
    }

    vm->stack[addr] = value;
}

static inline void ADD(void) {
    Box roperand = pop();
    Box loperand = pop();

    if (type(loperand) == VAL_INT && type(roperand) == VAL_INT) {
        int value = unwrapInt(loperand) + unwrapInt(roperand);
        push(createBox(&value, VAL_INT));
    } else {
        void *lval = &loperand;
        void *rval = &roperand;

        double castedValue;
        if (type(loperand) == VAL_INT) {
            castedValue = unwrapInt(loperand);
            lval = &castedValue;
        } else if (type(roperand) == VAL_INT) {
            castedValue = unwrapInt(roperand);
            rval = &castedValue;
        }

        double result = *(double *)lval + *(double *)rval;
        push(createBox(&result, VAL_FLOAT));
    }
}

static inline void SUB(void) {
    Box roperand = pop();
    Box loperand = pop();

    if (type(loperand) == VAL_INT && type(roperand) == VAL_INT) {
        int value = unwrapInt(loperand) - unwrapInt(roperand);
        push(createBox(&value, VAL_INT));
    } else {
        void *lval = &loperand;
        void *rval = &roperand;

        double castedValue;
        if (type(loperand) == VAL_INT) {
            castedValue = unwrapInt(loperand);
            lval = &castedValue;
        } else if (type(roperand) == VAL_INT) {
            castedValue = unwrapInt(roperand);
            rval = &castedValue;
        }

        double result = *(double *)lval - *(double *)rval;
        push(createBox(&result, VAL_FLOAT));
    }
}

static inline void MULT(void) {
    Box roperand = pop();
    Box loperand = pop();

    if (type(loperand) == VAL_INT && type(roperand) == VAL_INT) {
        int value = unwrapInt(loperand) * unwrapInt(roperand);
        push(createBox(&value, VAL_INT));
    } else {
        void *lval = &loperand;
        void *rval = &roperand;

        double castedValue;
        if (type(loperand) == VAL_INT) {
            castedValue = unwrapInt(loperand);
            lval = &castedValue;
        } else if (type(roperand) == VAL_INT) {
            castedValue = unwrapInt(roperand);
            rval = &castedValue;
        }

        double result = *(double *)lval * *(double *)rval;
        push(createBox(&result, VAL_FLOAT));
    }
}

static inline void DIV(void) {
    Box roperand = pop();
    Box loperand = pop();

    if (type(loperand) == VAL_INT && type(roperand) == VAL_INT) {
        int value = unwrapInt(loperand) / unwrapInt(roperand);
        push(createBox(&value, VAL_INT));
    } else {
        void *lval = &loperand;
        void *rval = &roperand;

        double castedValue;
        if (type(loperand) == VAL_INT) {
            castedValue = unwrapInt(loperand);
            lval = &castedValue;
        } else if (type(roperand) == VAL_INT) {
            castedValue = unwrapInt(roperand);
            rval = &castedValue;
        }

        double result = *(double *)lval / *(double *)rval;
        push(createBox(&result, VAL_FLOAT));
    }
}

static inline int CJMP(int operand) {
    Box value = pop();

    int shouldJump = unwrapInt(value);


    if (shouldJump || vm->conditionBreaker) {
        vm->pc += operand;
        return 1;
    };
    vm->conditionBreaker = 1;
    return 0;
}

static inline void compareInt(int operand, int remainder) {
    int boolVal;

    switch (operand) {
    case CMP_EQ:
        boolVal = remainder == 0;
        break;
    case CMP_NE:
        boolVal = remainder != 0;
        break;
    case CMP_GE:
        boolVal = remainder >= 0;
        break;
    case CMP_GT:
        boolVal = remainder > 0;
        break;
    case CMP_LE:
        boolVal = remainder <= 0;
        break;
    case CMP_LT:
        boolVal = remainder < 0;
        break;
    default:
        fprintf(stderr, "unexpected branch");
        exit(1);
    }

    write(vm->sp, createBox(&boolVal, VAL_INT));
}

static inline void compareFloat(int operand, double remainder) {
    int boolVal;

    switch (operand) {
    case CMP_EQ:
        boolVal = remainder == 0;
        break;
    case CMP_NE:
        boolVal = remainder != 0;
        break;
    case CMP_GE:
        boolVal = remainder >= 0;
        break;
    case CMP_GT:
        boolVal = remainder > 0;
        break;
    case CMP_LE:
        boolVal = remainder <= 0;
        break;
    case CMP_LT:
        boolVal = remainder < 0;
        break;
    default:
        fprintf(stderr, "unexpected branch");
        exit(1);
    }

    write(vm->sp, createBox(&boolVal, VAL_INT));
}

static inline void CMP(int operand) {
    SUB();

    Box value = read(vm->sp);

    switch (type(value)) {
    case VAL_INT:
        compareInt(operand, unwrapInt(value));
        break;
    case VAL_FLOAT:
        compareFloat(operand, value);
        break;
    default:
        fprintf(stderr, "weird branch");
        exit(1);
    }
}

static inline void NOT(void) {
    Box value = pop();

    switch (type(value)) {
    case VAL_INT: {
        int v = !unwrapInt(value);
        push(createBox(&v, VAL_INT));
        break;
    }
    case VAL_FLOAT: {
        double v = !value;
        push(createBox(&v, VAL_INT));
        break;
    }
    default:
        break;
    }
}

void executeProgram(void) {
    while (vm->pc < vm->programLength) {
        switch (vm->program[vm->pc].type) {
        case INST_PUSH:
            push(vm->program[vm->pc].operand);
            break;
        case INST_ADD:
            ADD();
            break;
        case INST_SUB:
            SUB();
            break;
        case INST_MULT:
            MULT();
            break;
        case INST_DIV:
            DIV();
            break;
        case INST_NOT:
            NOT();
            break;
        case INST_CMP:
            CMP(unwrapInt(vm->program[vm->pc].operand));
            break;
        case INST_JMP:
            break;
        case INST_CJMP:
            if (CJMP(unwrapInt(vm->program[vm->pc].operand))) {
                continue;
            };
            break;
        case INST_RESET_CB:
            vm->conditionBreaker = 0;
            break;
        }
        
        vm->pc += 1;
    }

    memoryDump();
}

char* stringifyInst(InstType type) {
    switch (type) {
    case INST_JMP: return "INST_JMP";
    case INST_CJMP: return "INST_CJMP";
    case INST_PUSH: return "INST_PUSH";
    case INST_ADD: return "INST_ADD";
    case INST_SUB: return "INST_SUB";
    case INST_MULT: return "INST_MULT";
    case INST_DIV: return "INST_DIV";
    case INST_CMP: return "INST_CMP";
    case INST_NOT: return "INST_NOT";
    case INST_RESET_CB: return "INST_RESET_CB";
    }
}

void printBox(Box box) {
    switch (type(box)) {
    case VAL_INT:
        printf("%d", unwrapInt(box));
        break;
    case VAL_FLOAT:
        printf("%f", box);
        break;
    default:
        break;
    }
}

void programDump(void) {
    printf("===== DISASSEMBLY =====\n\n");

    for (int i = 0; i < vm->programLength; i++) {
        printf("PC: (0x%04X) %s ", i, stringifyInst(vm->program[i].type));
        if (vm->program[i].operand) {
            printBox(vm->program[i].operand);
        }
        printf("\n");
    }
    printf("\n");
}

void memoryDump(void) {
    printf("===== RUNTIME STACK =====\n\n");

    if (vm->sp < 0) {
        printf("[EMPTY]\n");
    } else {
        printf("> ");
        for (int i = vm->sp; i >= 0; i--) {
            printBox(vm->stack[i]);
            printf(" ");
        }
        printf("\n");
    }
    printf("\n");
}
