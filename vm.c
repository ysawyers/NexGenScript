#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vm.h"

#define STACK_PUSH(value) vm->sp += 1;               \
                          vm->stack[vm->sp] = value; \

#define STACK_POP(value)  value = vm->stack[vm->sp]; \
                          vm->sp -= 1;                \

#define CALLSTACK_PUSH(value) vm->csp += 1;                   \
                              vm->callStack[vm->csp] = value; \

VM *vm;

void initVM(Inst *program, size_t length) {
    vm = malloc(sizeof(VM));
    vm->csp = -1;
    vm->sp = -1;
    vm->programLength = length;
    vm->program = program;
    vm->conditionBreaker = 0;
}

void freeVM(void) {
    free(vm->program);
    free(vm);
}

// TODO: Need to support comparison between floats
static inline void CMP(int operand) {
    STACK_POP(Box roperand);
    STACK_POP(Box loperand);

    if (type(roperand) == VAL_INT && type(loperand) == VAL_INT) {
            int result;

            switch(operand) {
            case CMP_EQ:
                result = unwrapInt(loperand) == unwrapInt(roperand);
                STACK_PUSH(createBox(&result, VAL_INT));
                break;
            case CMP_NE:
                result = unwrapInt(loperand) != unwrapInt(roperand);
                STACK_PUSH(createBox(&result, VAL_INT));
                break;
            case CMP_GE:
                result = unwrapInt(loperand) >= unwrapInt(roperand);
                STACK_PUSH(createBox(&result, VAL_INT));
                break;
            case CMP_GT:
                result = unwrapInt(loperand) > unwrapInt(roperand);
                STACK_PUSH(createBox(&result, VAL_INT));
                break;
            case CMP_LE:
                result = unwrapInt(loperand) <= unwrapInt(roperand);
                STACK_PUSH(createBox(&result, VAL_INT));
                break;
            case CMP_LT:
                result = unwrapInt(loperand) < unwrapInt(roperand);
                STACK_PUSH(createBox(&result, VAL_INT));
                break;
            }
    }
}

void executeProgram(void) {
    while (vm->pc < vm->programLength) {
        switch (vm->program[vm->pc].type) {
        case INST_PUSH:
            STACK_PUSH(vm->program[vm->pc].operand);
            break;
        case INST_ADD: {
            STACK_POP(Box roperand);
            STACK_POP(Box loperand);

            if (type(loperand) == VAL_INT && type(roperand) == VAL_INT) {
                int value = unwrapInt(loperand) + unwrapInt(roperand);
                STACK_PUSH(createBox(&value, VAL_INT));
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
                STACK_PUSH(createBox(&result, VAL_FLOAT));
            }
            break;
        }
        case INST_SUB: {
            STACK_POP(Box roperand);
            STACK_POP(Box loperand);

            if (type(loperand) == VAL_INT && type(roperand) == VAL_INT) {
                int value = unwrapInt(loperand) - unwrapInt(roperand);
                STACK_PUSH(createBox(&value, VAL_INT));
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
                STACK_PUSH(createBox(&result, VAL_FLOAT));
            }
            break;
        }
        case INST_MULT: {
            STACK_POP(Box roperand);
            STACK_POP(Box loperand);

            if (type(loperand) == VAL_INT && type(roperand) == VAL_INT) {
                int value = unwrapInt(loperand) * unwrapInt(roperand);
                STACK_PUSH(createBox(&value, VAL_INT));
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
                STACK_PUSH(createBox(&result, VAL_FLOAT));
            }
            break;
        }
        case INST_DIV: {
            STACK_POP(Box roperand);
            STACK_POP(Box loperand);

            if (type(loperand) == VAL_INT && type(roperand) == VAL_INT) {
                int value = unwrapInt(loperand) / unwrapInt(roperand);
                STACK_PUSH(createBox(&value, VAL_INT));
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
                STACK_PUSH(createBox(&result, VAL_FLOAT));
            }
            break;
        }
        case INST_LOGICAL_NOT: {
            STACK_POP(Box value);

            switch (type(value)) {
            case VAL_INT: {
                int v = !unwrapInt(value);
                STACK_PUSH(createBox(&v, VAL_INT));
                break;
            }
            case VAL_FLOAT: {
                double v = !value;
                STACK_PUSH(createBox(&v, VAL_INT));
                break;
            }
            }
            break;
        }
        case INST_CMP:
            CMP(unwrapInt(vm->program[vm->pc].operand));
            break;
        case INST_CALL: {
            int nextInstruction = vm->pc + 1;
            
            CALLSTACK_PUSH(createBox(&nextInstruction, VAL_INT));
            CALLSTACK_PUSH(createBox(&vm->sp, VAL_INT));

            vm->pc = unwrapInt(vm->program[vm->pc].operand);
            continue;
        }
        case INST_RET: {
            STACK_POP(Box returnedValue);

            vm->sp = unwrapInt(vm->callStack[vm->csp]);
            vm->csp -= 1;
            vm->pc = unwrapInt(vm->callStack[vm->csp]);
            vm->csp -= 1;

            int numberOfArgs = unwrapInt(vm->callStack[vm->csp]);
            vm->csp -= 1;
            vm->csp -= numberOfArgs;

            STACK_PUSH(returnedValue);
            continue;
        }
        case INST_JMP:
            vm->pc += unwrapInt(vm->program[vm->pc].operand);
            continue;
        case INST_PUSH_ARG:
            vm->csp += 1;
            STACK_POP(vm->callStack[vm->csp]);
            break;
        case INST_FETCH_ARG: {
            int index = unwrapInt(vm->program[vm->pc].operand);
            int basept = vm->csp - 2 - unwrapInt(vm->callStack[vm->csp - 2]);
            STACK_PUSH(vm->callStack[basept + index]);
            break;
        }
        case INST_CJMP: {
            STACK_POP(Box value);

            int shouldJump = unwrapInt(value);
            if (shouldJump || vm->conditionBreaker) {
                vm->pc += unwrapInt(vm->program[vm->pc].operand);
                continue;
            };
            break;
        }
        case INST_SET_CB:
            vm->conditionBreaker = 1;
            break;
        case INST_UNSET_CB:
            vm->conditionBreaker = 0;
            break;
        }

        if (vm->csp >= CALLSTACK_MAX_SIZE) {
            fprintf(stderr, "runtime error: Maximum callstack size exceeded");
            exit(1);
        }

        vm->pc += 1;
    }

    memoryDump();
    dumpCallStack();
}

char* stringifyInst(InstType type) {
    switch (type) {
    case INST_FETCH_ARG: return "INST_FETCH_ARG";
    case INST_PUSH_ARG: return "INST_PUSH_ARG";
    case INST_CALL: return "INST_CALL";
    case INST_RET: return "INST_RET";
    case INST_JMP: return "INST_JMP";
    case INST_CJMP: return "INST_CJMP";
    case INST_PUSH: return "INST_PUSH";
    case INST_ADD: return "INST_ADD";
    case INST_SUB: return "INST_SUB";
    case INST_MULT: return "INST_MULT";
    case INST_DIV: return "INST_DIV";
    case INST_CMP: return "INST_CMP";
    case INST_LOGICAL_NOT: return "INST_LOGICAL_NOT";
    case INST_SET_CB: return "INST_SET_CB";
    case INST_UNSET_CB: return "INST_UNSET_CB";
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

void dumpCallStack(void) {
    printf("===== CALL STACK =====\n\n");

    if (vm->csp < 0) {
        printf("[EMPTY]\n");
    } else {
        printf("> ");
        for (int i = vm->csp; i >= 0; i--) {
            printBox(vm->callStack[i]);
            printf(" ");
        }
        printf("\n");
    }
    printf("\n");
}
