#ifndef VALUE_H
#define VALUE_H

typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_BOOL,
} ValueType;

double encodeTaggedLiteral(void *value, ValueType type);
ValueType type(double box);
int unwrapInt(double *box);

#endif
