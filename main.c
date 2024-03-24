#include <stdlib.h>
#include <stdio.h>
#include "scanner.h"
#include "vm.h"
#include "compiler.h"

char* readFile(const char *filepath) {
    FILE *file;

    file = fopen(filepath, "rb");
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(fileSize + 1);
    unsigned long bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';

    fclose(file);

    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("missing path to .ngs file to compile\n");
        return 1;
    }

    char *sourceFile = readFile(argv[1]);
    scannerInitialize(sourceFile);

    int programSize = 0;
    Inst *program = compile(&programSize);

    free(sourceFile);

    initVM(program, programSize);

    programDump();
    executeProgram();

    freeVM();

    return 0;
}
