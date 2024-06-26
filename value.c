#include <stdlib.h>
#include <stdio.h>
#include "value.h"

inline Box createBox(void *value, ValueType type) {
    Box box = {.float64 = NAN};

    switch(type) {
    case VAL_FLOAT:
        box.float64 = *((double *)value);
        break;
    case VAL_INT:
        box.int32 = *((int32_t *)value);
        ((uint8_t *)(&box))[6] = 0xF8 | type;
        break;
    case VAL_STRING:
        box.obj = (uint64_t)value;
        ((uint8_t *)(&box))[6] = 0xF8 | type;
        break;
    }

    return box;
}
