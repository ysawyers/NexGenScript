#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"

VM* initVM(Inst *program, size_t length) {
    VM *vm;

    vm = malloc(sizeof(VM));
    vm->sp = -1;
    vm->programLength = length;
    vm->program = program;

    return vm;
}

void freeVM(VM *vm) {
    free(vm->program);
    free(vm);
}

void executeProgram(VM *vm) {
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
            if (vm->sp < 1) {
                fprintf(stderr, "runtime error: stack underflow\n");
                exit(1);
            }
            vm->sp -= 1;
            vm->stack[vm->sp] = vm->stack[vm->sp] + vm->stack[vm->sp + 1];
            break;
        case INST_SUB:
            if (vm->sp < 1) {
                fprintf(stderr, "runtime error: stack underflow\n");
                exit(1);
            }
            vm->sp -= 1;
            vm->stack[vm->sp] = vm->stack[vm->sp] - vm->stack[vm->sp + 1];
            break;
        case INST_MULT:
            if (vm->sp < 1) {
                fprintf(stderr, "runtime error: stack underflow\n");
                exit(1);
            }
            vm->sp -= 1;
            vm->stack[vm->sp] = vm->stack[vm->sp] * vm->stack[vm->sp + 1];
            break;
        case INST_DIV:
            if (vm->sp < 1) {
                fprintf(stderr, "runtime error: stack underflow\n");
                exit(1);
            }

            if (vm->stack[vm->sp] == 0) {
                fprintf(stderr, "runtime error: division by 0\n");
                exit(1);
            }

            vm->sp -= 1;
            vm->stack[vm->sp] = vm->stack[vm->sp] / vm->stack[vm->sp + 1];
            break;
        case INST_CMP:
            if (vm->sp < 1) {
                fprintf(stderr, "runtime error: stack underflow\n");
                exit(1);
            }

            vm->sp -= 1;
            Word result = vm->stack[vm->sp] - vm->stack[vm->sp + 1];

            switch (vm->program[vm->pc].operand) {
            case CMP_EQ:
                vm->stack[vm->sp] = result == 0;
                break;
            case CMP_NE:
                vm->stack[vm->sp] = result != 0;
                break;
            case CMP_GE:
                vm->stack[vm->sp] = result >= 0;
                break;
            case CMP_GT:
                vm->stack[vm->sp] = result > 0;
                break;
            case CMP_LE:
                vm->stack[vm->sp] = result <= 0;
                break;
            case CMP_LT:
                vm->stack[vm->sp] = result < 0;
                break;
            default:
                fprintf(stderr, "unexpected branch");
                exit(1);
            }
            break;
        case INST_JMP:
            // TODO
            break;
        case INST_CJMP:
            if (vm->sp < 0) {
                fprintf(stderr, "runtime error: stack underflow\n");
                exit(1);
            }

            if (vm->stack[vm->sp]) {
                vm->pc += vm->program[vm->pc].operand;
                vm->sp -= 1;
                continue;
            }
            vm->sp -= 1;
            break;
        }

        vm->pc += 1;
    }
}

const char* stringifyInst(InstType type) {
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

void programDump(VM *vm) {
    printf("PROGRAM:\n\n");

    for (int i = 0; i < vm->programLength; i++) {
        printf("%02X %08llX %s", vm->program[i].type, vm->program[i].operand, stringifyInst(vm->program[i].type));

        if (vm->program[i].operand) {
            printf(" %lld", vm->program[i].operand);
        }
        printf("\n");
    }
}

void memoryDump(VM *vm) {
    printf("STACK:\n\n");

    if (vm->sp < 0) {
        printf("[EMPTY]\n");
    } else {
        for (int i = 0; i <= vm->sp; i++) {
            printf("%lld ", vm->stack[i]);
        }
        printf("\n");
    }
}
