#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "value.h"

Box createBox(void *value, ValueType type) {
    Box box = NAN;

    switch(type) {
    case VAL_FLOAT:
        return *((Box *)value);
    case VAL_INT:
        if (*(int32_t *)value > INT32_MAX) return (Box)(*(int32_t *)value);
        ((uint8_t *)(&box))[6] = 0xF8 + VAL_INT;
        memcpy(&box, (int32_t *)value, sizeof(int32_t));
        break;
    }

    return box;
}

ValueType type(Box box) {
    if (!isnan(box)) return VAL_FLOAT;

    switch (((uint8_t *)(&box))[6] - 0xF8) {
    case VAL_INT: return VAL_INT;
    }

    return 0;
}

int32_t unwrapInt(Box box) {
    int v = 0;
    memcpy(&v, &box, sizeof(int32_t));
    return v;
}
