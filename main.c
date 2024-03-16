#include <stdlib.h>
#include <stdio.h>
#include "scanner.h"
#include "vm.h"
#include "compiler.h"

static char* readFile(const char *filepath) {
    FILE *file;
    unsigned long fileSize;

    file = fopen(filepath, "rb");

    fseek(file, 0L, SEEK_END);
    fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(fileSize + 1);
    unsigned long bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';

    fclose(file);

    return buffer;
}

int main(int argc, char *argv[]) {
    VM *vm;
    Inst *program;
    char *sourceFile;
    int programLength = 0;

    if (argc < 2) {
        printf("missing path to .ngs file to compile\n");
        return 1;
    }

    sourceFile = readFile(argv[1]);
    scannerInitialize(sourceFile);
    program = compile(&programLength);

    free(sourceFile);

    vm = initVM(program, programLength);
    programDump(vm);
    executeProgram(vm);
    memoryDump(vm);
    freeVM(vm);

    return 0;
}
