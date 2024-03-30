#ifndef VALUE_H
#define VALUE_H

#include <math.h>

#define TYPE(box) (!isnan(box.float64) ? VAL_FLOAT : ((uint8_t *)&box)[6] & 0x7)

typedef struct {
    size_t length;
    void *ref;
} Object;

typedef union {
    double  float64;
    int64_t obj : 48;
    int32_t int32;
} Box;

typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
} ValueType;

Box createBox(void *value, ValueType type);

#endif
