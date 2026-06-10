#ifndef ASSERTION_H
#define ASSERTION_H

#include <stdbool.h>

#include "models.h"


typedef enum{
    VALUE_NONE,
    VALUE_INT,
    VALUE_STRING,
    VALUE_BOOL,
    VALUE_DOUBLE
}ValueType;

typedef struct 
{
    ValueType type;
    union{
        int int_value;
        const char* string_value;
        int bool_value;
        double double_value;
    } as;
}Value;


typedef enum{
    ASSERT_STATUS_EQ
}AssertionType;

typedef struct{
    AssertionType type;    
    Value expected;
}Assertion;

typedef struct {
    AssertionType type;
    bool passed;
    Value expected;
    Value actual;
    const char* message;
} AssertionResult;

AssertionResult eval_assertion(const Response* resp, const Assertion* as);

#endif