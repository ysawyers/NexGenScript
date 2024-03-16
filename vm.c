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
    for (int i = 0; i < vm->programLength; i++) {
        switch (vm->program[i].type) {
        case INST_PUSH:
            if (vm->sp < MEM_SIZE) {
                vm->sp += 1;
                vm->stack[vm->sp] = vm->program[i].operand;
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
        }
    }
}

const char* stringifyInst(Inst_Type type) {
    switch (type) {
    case INST_PUSH: return "INST_PUSH";
    case INST_ADD: return "INST_ADD";
    case INST_SUB: return "INST_SUB";
    case INST_MULT: return "INST_MULT";
    case INST_DIV: return "INST_DIV";
    }
}

void programDump(VM *vm) {
    printf("PROGRAM:\n\n");

    for (int i = 0; i < vm->programLength; i++) {
        printf("%s", stringifyInst(vm->program[i].type));

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
