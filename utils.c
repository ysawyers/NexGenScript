#include <stdio.h>
#include "utils.h"

void* safe_malloc(size_t size) {
    void *memory = malloc(size);
    if (memory == NULL) {
        fprintf(stderr, "ERROR: Cannot allocate more memory");
        exit(1);
    }
    return memory;
}

void* safe_calloc(size_t count, size_t size) {
    void *memory = calloc(count, size);
    if (memory == NULL) {
        fprintf(stderr, "ERROR: Cannot allocate more memory");
        exit(1);
    }
    return memory;
}
