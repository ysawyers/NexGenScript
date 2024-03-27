#ifndef VALUE_H
#define VALUE_H

typedef union {
    double  float64;
    int32_t int32;
} Box;

typedef enum {
    VAL_INT,
    VAL_FLOAT,
} ValueType;

Box createBox(void *value, ValueType type);
ValueType type(Box box);

#endif
