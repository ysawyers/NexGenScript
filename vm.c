#include <stdio.h>
#include <stdlib.h>
#include "vm.h"

#define STACK_PUSH(value) vm->sp += 1;                      \
                          vm->operandStack[vm->sp] = value; \

#define STACK_POP(value)  value = vm->operandStack[vm->sp]; \
                          vm->sp -= 1;                      \

#define CALLSTACK_PUSH(value) vm->csp += 1;                   \
                              vm->callStack[vm->csp] = value; \

VM *vm;

void initVM(Inst *program, size_t length) {
    vm = calloc(1, sizeof(VM));
    vm->csp = -1;
    vm->sp = -1;
    vm->programLength = length;
    vm->program = program;
}

void freeVM(void) {
    free(vm->program);
    free(vm);
}

void executeProgram(void) {
    while (vm->pc < vm->programLength) {
        InstType opcode = vm->program[vm->pc].type;
        Box operand = vm->program[vm->pc].operand;

        switch (opcode) {
        case INST_STACK_PUSH:
            STACK_PUSH(operand);
            break;
        case INST_ADD: {
            STACK_POP(Box roperand);
            STACK_POP(Box loperand);

            if (TYPE(loperand) == VAL_INT && TYPE(roperand) == VAL_INT) {
                int lv = loperand.int32;
                int rv = roperand.int32;
                int result = lv + rv;

                if ((lv < 0 && rv < 0 && result >= 0) || (lv > 0 && rv > 0 && result <= 0)) {
                    double promotion = (double)lv + (double)rv;
                    STACK_PUSH(createBox(&promotion, VAL_FLOAT));
                } else {
                    STACK_PUSH(createBox(&result, VAL_INT));
                }
            } else {
                double lv = loperand.float64;
                double rv = roperand.float64;

                if (TYPE(loperand) == VAL_INT) {
                    lv = (double)loperand.int32;
                } else if (TYPE(roperand) == VAL_INT) {
                    rv = (double)roperand.int32;
                }

                double result = lv + rv;
                STACK_PUSH(createBox(&result, VAL_FLOAT));
            }
            break;
        }
        case INST_SUB: {
            STACK_POP(Box roperand);
            STACK_POP(Box loperand);

            if (TYPE(loperand) == VAL_INT && TYPE(roperand) == VAL_INT) {
                int lv = loperand.int32;
                int rv = roperand.int32;
                int result = lv - rv;

                if (result > lv) {
                    double promotion = (double)lv - (double)rv;
                    STACK_PUSH(createBox(&promotion, VAL_FLOAT));
                } else {
                    STACK_PUSH(createBox(&result, VAL_INT));
                }
            } else {
                double lv = loperand.float64;
                double rv = roperand.float64;

                if (TYPE(loperand) == VAL_INT) {
                    lv = (double)loperand.int32;
                } else if (TYPE(roperand) == VAL_INT) {
                    rv = (double)roperand.int32;
                }

                double result = lv - rv;
                STACK_PUSH(createBox(&result, VAL_FLOAT));
            }
            break;
        }
        case INST_MULT: {
            STACK_POP(Box roperand);
            STACK_POP(Box loperand);

            if (TYPE(loperand) == VAL_INT && TYPE(roperand) == VAL_INT) {
                int lv = loperand.int32;
                int rv = roperand.int32;
                int result = lv * rv;

                if ((lv < 0 && rv < 0 && result >= 0) || (lv > 0 && rv > 0 && result <= 0)) {
                    double promotion = (double)lv * (double)rv;
                    STACK_PUSH(createBox(&promotion, VAL_FLOAT));
                } else {
                    STACK_PUSH(createBox(&result, VAL_INT));
                }
            } else {
                double lv = loperand.float64;
                double rv = roperand.float64;

                if (TYPE(loperand) == VAL_INT) {
                    lv = (double)loperand.int32;
                } else if (TYPE(roperand) == VAL_INT) {
                    rv = (double)roperand.int32;
                }

                double result = lv * rv;
                STACK_PUSH(createBox(&result, VAL_FLOAT));
            }
            break;
        }
        case INST_DIV: {
            STACK_POP(Box roperand);
            STACK_POP(Box loperand);

            if (TYPE(loperand) == VAL_INT && TYPE(roperand) == VAL_INT) {
                int result = loperand.int32 / roperand.int32;
                STACK_PUSH(createBox(&result, VAL_INT));
            } else {
                double lv = loperand.float64;
                double rv = roperand.float64;

                if (TYPE(loperand) == VAL_INT) {
                    lv = (double)loperand.int32;
                } else if (TYPE(roperand) == VAL_INT) {
                    rv = (double)roperand.int32;
                }

                double result = lv / rv;
                STACK_PUSH(createBox(&result, VAL_FLOAT));
            }
            break;
        }
        case INST_LOGICAL_NOT: {
            int result;

            STACK_POP(Box value);
            switch (TYPE(value)) {
            case VAL_INT: {
                result = !value.int32;
                break;
            }
            case VAL_FLOAT: {
                result = !value.float64;
                break;
            }
            }
            STACK_PUSH(createBox(&result, VAL_INT));
            break;
        }
        case INST_CMP: {
            int result;

            STACK_POP(Box roperand);
            STACK_POP(Box loperand);

            if (TYPE(roperand) == VAL_INT && TYPE(loperand) == VAL_INT) {
                    switch(operand.int32) {
                    case CMP_EQ:
                        result = loperand.int32 == roperand.int32;
                        break;
                    case CMP_NE:
                        result = loperand.int32 != roperand.int32;
                        break;
                    case CMP_GE:
                        result = loperand.int32 >= roperand.int32;
                        break;
                    case CMP_GT:
                        result = loperand.int32 > roperand.int32;
                        break;
                    case CMP_LE:
                        result = loperand.int32 <= roperand.int32;
                        break;
                    case CMP_LT:
                        result = loperand.int32 < roperand.int32;
                        break;
                    }
            } else {
                float lv = loperand.float64;
                float rv = roperand.float64;

                if (TYPE(loperand) == VAL_INT) {
                    lv = (double)loperand.int32;
                } else if (TYPE(roperand) == VAL_INT) {
                    rv = (double)roperand.int32;
                }

                switch(operand.int32) {
                case CMP_EQ:
                    result = lv == rv;
                    break;
                case CMP_NE:
                    result = lv != rv;
                    break;
                case CMP_GE:
                    result = lv >= rv;
                    break;
                case CMP_GT:
                    result = lv > rv;
                    break;
                case CMP_LE:
                    result = lv <= rv;
                    break;
                case CMP_LT:
                    result = lv < rv;
                    break;
                }
            }
            STACK_PUSH(createBox(&result, VAL_INT));
            break;
        }
        case INST_CALL: {
            int nextInstruction = vm->pc + 1;

            CALLSTACK_PUSH(createBox(&nextInstruction, VAL_INT));
            CALLSTACK_PUSH(createBox(&vm->sp, VAL_INT));

            vm->pc = operand.int32;
            continue;
        }
        case INST_RET: {
            STACK_POP(Box returnedValue);

            vm->sp = vm->callStack[vm->csp].int32;
            vm->csp -= 1;
            vm->pc = vm->callStack[vm->csp].int32;
            vm->csp -= 1;

            int numberOfArgs = vm->callStack[vm->csp].int32;
            vm->csp -= 1;
            vm->csp -= numberOfArgs;

            STACK_PUSH(returnedValue);
            continue;
        }
        case INST_JMP:
            vm->pc += operand.int32;
            continue;
        case INST_PUSH_ARG:
            vm->csp += 1;
            STACK_POP(vm->callStack[vm->csp]);
            break;
        case INST_FETCH_ARG: {
            int index = operand.int32;
            int basept = vm->csp - 2 - vm->callStack[vm->csp - 2].int32;
            STACK_PUSH(vm->callStack[basept + index]);
            break;
        }
        case INST_JMP_IF_NOT: {
            STACK_POP(Box value);

            int shouldJump = !value.int32;
            if (shouldJump || vm->conditionBreaker) {
                vm->pc += operand.int32;
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
        case INST_STACK_SWEEP:
            for (int i = 0; i < operand.int32; i++) {
                Box box = vm->operandStack[vm->sp];
                if (TYPE(box) == VAL_STRING) {
                    free((char *)box.obj);
                };
                vm->sp -= 1;
            }
            break;
        case INST_FETCH_VAR: {
            Box value = vm->operandStack[operand.int32];
            STACK_PUSH(value);
            break;
        }
        case INST_ASSIGN_VAR: {
            STACK_POP(vm->operandStack[operand.int32]);
            break;
        }
        }

        if (vm->csp >= CALLSTACK_MAX_SIZE) {
            fprintf(stderr, "runtime error: Maximum callstack size exceeded");
            exit(1);
        }

        vm->pc += 1;
    }

    dumpOperandStack();
    dumpCallStack();
}

char* stringifyInst(InstType type) {
    switch (type) {
    case INST_ASSIGN_VAR: return "INST_ASSIGN_VAR";
    case INST_STACK_SWEEP: return "INST_STACK_SWEEP";
    case INST_FETCH_ARG: return "INST_FETCH_ARG";
    case INST_PUSH_ARG: return "INST_PUSH_ARG";
    case INST_CALL: return "INST_CALL";
    case INST_RET: return "INST_RET";
    case INST_JMP: return "INST_JMP";
    case INST_JMP_IF_NOT: return "INST_JMP_IF_NOT";
    case INST_STACK_PUSH: return "INST_PUSH";
    case INST_ADD: return "INST_ADD";
    case INST_SUB: return "INST_SUB";
    case INST_MULT: return "INST_MULT";
    case INST_DIV: return "INST_DIV";
    case INST_CMP: return "INST_CMP";
    case INST_LOGICAL_NOT: return "INST_LOGICAL_NOT";
    case INST_SET_CB: return "INST_SET_CB";
    case INST_UNSET_CB: return "INST_UNSET_CB";
    case INST_FETCH_VAR: return "INST_FETCH_VAR";
    }
}

void printBox(Box box) {
    switch (TYPE(box)) {
    case VAL_INT:
        printf("%d", box.int32);
        break;
    case VAL_FLOAT:
        printf("%f", box.float64);
        break;
    case VAL_STRING:
        printf("%s", (char *)box.obj);
        break;
    default:
        break;
    }
}

void dumpProgram(void) {
    printf("===== DISASSEMBLY =====\n\n");

    for (int i = 0; i < vm->programLength; i++) {
        printf("PC: (0x%04X) %s ", i, stringifyInst(vm->program[i].type));
        if (vm->program[i].operand.float64) {
            printBox(vm->program[i].operand);
        }
        printf("\n");
    }
    printf("\n");
}

void dumpOperandStack(void) {
    printf("===== OPERAND STACK =====\n\n");

    if (vm->sp < 0) {
        printf("[EMPTY]\n");
    } else {
        printf("> ");
        for (int i = vm->sp; i >= 0; i--) {
            printBox(vm->operandStack[i]);
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
