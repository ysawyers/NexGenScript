#ifndef VALUE_H
#define VALUE_H

typedef double Box;

typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_BOOL,
} ValueType;

double createBox(void *value, ValueType type);
ValueType type(Box box);
int32_t unwrapInt(Box box);

#endif
