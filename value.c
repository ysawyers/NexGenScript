#include <stdlib.h>
#include <math.h>
#include "value.h"

inline Box createBox(void *value, ValueType type) {
    Box box = {.float64 = NAN};

    switch(type) {
    case VAL_FLOAT:
        box.float64 = *((double *)value);
        break;
    case VAL_INT:
        ((uint8_t *)(&box))[6] = 0xF8 | VAL_INT;
        box.int32 = *((int32_t *)value);
        break;
    }

    return box;
}

inline ValueType type(Box box) {
    if (!isnan(box.float64)) return VAL_FLOAT;

    switch (((uint8_t *)(&box))[6] & 0x7) {
    case VAL_INT: return VAL_INT;
    }

    return 0;
}
