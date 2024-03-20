#include <stdio.h>
#include <string.h>
#include <math.h>
#include "vm.h"

// NOTE: assumes on little endian machine

VM *vm;

void initVM(Inst *program, size_t length) {
    vm = malloc(sizeof(VM));
    vm->sp = -1;
    vm->programLength = length;
    vm->program = program;
}

void freeVM(void) {
    free(vm->program);
    free(vm);
}

Word read(int addr) {
    if (addr > vm->sp || addr >= MEM_SIZE || addr < 0) {
        fprintf(stderr, "runtime error: invalid READ access\n");
        exit(1);
    }
    return vm->stack[addr];
}

void write(int addr, Word value) {
    if (addr > vm->sp || addr >= MEM_SIZE || addr < 0) {
        fprintf(stderr, "runtime error: invalid WRITE access\n");
        exit(1);
    }
    vm->stack[addr] = value;
}

static void ADD(int lopaddr, int ropaddr) {
    Word loperand = read(lopaddr);
    Word roperand = read(ropaddr);
    vm->sp -= 1;

    if (type(loperand) == VAL_INT) {
        int32_t lval = unwrapInt(&loperand);

        if (type(roperand) == VAL_INT) {
            int32_t v = lval + unwrapInt(&roperand);
            write(vm->sp, encodeTaggedLiteral(&v, VAL_INT));
        } else if (type(roperand) == VAL_FLOAT) {
            Word v = (Word)lval + roperand;
            write(vm->sp, encodeTaggedLiteral(&v, VAL_FLOAT));
        }
    } else if (type(loperand) == VAL_FLOAT) {
        Word lval = loperand;

        if (type(roperand) == VAL_INT) {
            Word v = lval + (Word)unwrapInt(&roperand);
            write(vm->sp, encodeTaggedLiteral(&v, VAL_FLOAT));
        } else if (type(roperand) == VAL_FLOAT) {
            Word v = lval + roperand;
            write(vm->sp, encodeTaggedLiteral(&v, VAL_FLOAT));
        }
    }
}

static void SUB(int lopaddr, int ropaddr) {
    Word loperand = read(lopaddr);
    Word roperand = read(ropaddr);
    vm->sp -= 1;

    if (type(loperand) == VAL_INT) {
        int32_t lval = unwrapInt(&loperand);

        if (type(roperand) == VAL_INT) {
            int32_t v = lval - unwrapInt(&roperand);
            write(vm->sp, encodeTaggedLiteral(&v, VAL_INT));
        } else if (type(roperand) == VAL_FLOAT) {
            Word v = (Word)lval - roperand;
            write(vm->sp, encodeTaggedLiteral(&v, VAL_FLOAT));
        }
    } else if (type(loperand) == VAL_FLOAT) {
        Word lval = loperand;

        if (type(roperand) == VAL_INT) {
            Word v = lval - (Word)unwrapInt(&roperand);
            write(vm->sp, encodeTaggedLiteral(&v, VAL_FLOAT));
        } else if (type(roperand) == VAL_FLOAT) {
            Word v = lval - roperand;
            write(vm->sp, encodeTaggedLiteral(&v, VAL_FLOAT));
        }
    }
}

static void MULT(int lopaddr, int ropaddr) {
    Word loperand = read(lopaddr);
    Word roperand = read(ropaddr);
    vm->sp -= 1;

    if (type(loperand) == VAL_INT) {
        int32_t lval = unwrapInt(&loperand);

        if (type(roperand) == VAL_INT) {
            int32_t v = lval * unwrapInt(&roperand);
            write(vm->sp, encodeTaggedLiteral(&v, VAL_INT));
        } else if (type(roperand) == VAL_FLOAT) {
            Word v = (Word)lval * roperand;
            write(vm->sp, encodeTaggedLiteral(&v, VAL_FLOAT));
        }
    } else if (type(loperand) == VAL_FLOAT) {
        Word lval = loperand;

        if (type(roperand) == VAL_INT) {
            Word v = lval * (Word)unwrapInt(&roperand);
            write(vm->sp, encodeTaggedLiteral(&v, VAL_FLOAT));
        } else if (type(roperand) == VAL_FLOAT) {
            Word v = lval * roperand;
            write(vm->sp, encodeTaggedLiteral(&v, VAL_FLOAT));
        }
    }
}

static void DIV(int lopaddr, int ropaddr) {
    Word loperand = read(lopaddr);
    Word roperand = read(ropaddr);
    vm->sp -= 1;

    if (type(loperand) == VAL_INT) {
        int32_t lval = unwrapInt(&loperand);

        if (type(roperand) == VAL_INT) {
            int32_t v = lval / unwrapInt(&roperand);
            write(vm->sp, encodeTaggedLiteral(&v, VAL_INT));
        } else if (type(roperand) == VAL_FLOAT) {
            Word v = (Word)lval / roperand;
            write(vm->sp, encodeTaggedLiteral(&v, VAL_FLOAT));
        }
    } else if (type(loperand) == VAL_FLOAT) {
        Word lval = loperand;

        if (type(roperand) == VAL_INT) {
            Word v = lval / (Word)unwrapInt(&roperand);
            write(vm->sp, encodeTaggedLiteral(&v, VAL_FLOAT));
        } else if (type(roperand) == VAL_FLOAT) {
            Word v = lval / roperand;
            write(vm->sp, encodeTaggedLiteral(&v, VAL_FLOAT));
        }
    }
}

static void CJMP(int operand) {
    Word boxedValue = read(vm->sp);
    vm->sp -= 1;

    int v = unwrapInt(&boxedValue);
    if (v) {
        vm->pc += operand;
    }
}

static void compareInt(int operand, int remainder) {
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

    write(vm->sp, encodeTaggedLiteral(&boolVal, VAL_INT));
}

static void compareFloat(int operand, double remainder) {
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

    write(vm->sp, encodeTaggedLiteral(&boolVal, VAL_INT));
}

static void CMP(int operand) {
    SUB(vm->sp - 1, vm->sp);

    Word boxedValue = read(vm->sp);

    switch (type(boxedValue)) {
    case VAL_INT:
        compareInt(operand, unwrapInt(&boxedValue));
        break;
    case VAL_FLOAT:
        compareFloat(operand, boxedValue);
        break;
    default:
        fprintf(stderr, "weird branch");
        exit(1);
    }
}

void executeProgram(void) {
    while (vm->pc < vm->programLength) {
        switch (vm->program[vm->pc].type) {
        case INST_PUSH:
            if (vm->sp < MEM_SIZE) {
                vm->sp += 1;
                vm->stack[vm->sp] = vm->program[vm->pc].operand;
                break;
            }
            fprintf(stderr, "runtime error: stack overflow\n");
            exit(1);
        case INST_ADD:
            ADD(vm->sp - 1, vm->sp);
            break;
        case INST_SUB:
            SUB(vm->sp - 1, vm->sp);
            break;
        case INST_MULT:
            MULT(vm->sp - 1, vm->sp);
            break;
        case INST_DIV:
            DIV(vm->sp - 1, vm->sp);
            break;
        case INST_CMP:
            CMP(unwrapInt(&(vm->program[vm->pc].operand)));
            break;
        case INST_JMP:
            // TODO
            break;
        case INST_CJMP:
            CJMP(unwrapInt(&(vm->program[vm->pc].operand)));
            continue;
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
    }
}

void programDump(void) {
    printf("PROGRAM:\n\n");

    for (int i = 0; i < vm->programLength; i++) {
        printf("%02X %s", vm->program[i].type, stringifyInst(vm->program[i].type));

        if (vm->program[i].operand) {
            printf(" %d", unwrapInt(&vm->program[i].operand));
        }
        printf("\n");
    }
}

void memoryDump(void) {
    printf("STACK:\n\n");

    if (vm->sp < 0) {
        printf("[EMPTY]\n");
    } else {
        for (int i = 0; i <= vm->sp; i++) {
            printf("%f ", vm->stack[i]);
        }
        printf("\n");
    }
}
