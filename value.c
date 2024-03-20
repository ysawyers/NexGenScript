#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "value.h"

void printBits(double *box) {
    for (int i = 7; i >= 0; i--) {
        printf("%02X ", ((uint8_t *)(box))[i]);
    }
    printf("\n");
}

double encodeTaggedLiteral(void *value, ValueType type) {
    double box = NAN;

    switch(type) {
    case VAL_FLOAT:
        return *((double *)value);
    case VAL_BOOL:
        fprintf(stderr, "error: boolean not implemented yet!");
        exit(1);
    case VAL_INT:
        if (*(int32_t *)value > INT32_MAX) return (double)(*(int32_t *)value);
        ((uint8_t *)(&box))[6] = 0xF8 + VAL_INT;
        memcpy(&box, (int32_t *)value, sizeof(int32_t));
        break;
    }

    return box;
}

ValueType type(double box) {
    if (!isnan(box)) return VAL_FLOAT;

    switch (((uint8_t *)(&box))[6] - 0xF8) {
    case VAL_INT: return VAL_INT;
    case VAL_BOOL: return VAL_BOOL;
    default:
        fprintf(stderr, "error: type");
        exit(1);
    }

    return 0;
}

int32_t unwrapInt(double *box) {
    int v = 0;
    memcpy(&v, box, sizeof(int32_t));
    return v;
}
