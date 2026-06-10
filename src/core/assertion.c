#include <string.h>

#include "assertion.h"



static bool check_equal(Value val1, Value val2){
    if(val1.type != val2.type)
        return false;
        
    switch (val1.type)
    {

    case VALUE_INT:
        return val1.as.int_value == val2.as.int_value;    

    case VALUE_BOOL:
        return val1.as.bool_value == val2.as.bool_value;

    case VALUE_DOUBLE:
        return val1.as.double_value == val2.as.double_value;

    case VALUE_STRING:
        if (!val1.as.string_value || !val2.as.string_value)
            return false;
        return strcmp(val1.as.string_value, val2.as.string_value) == 0;
    
    case VALUE_NONE:
    default:
        return false;
    }

    return false;
}

AssertionResult eval_assertion(const Response* resp, const Assertion* as){
    AssertionResult as_result = {0};

    if (!resp || !as){
        as_result.passed = false;
        as_result.message = "Invalid assertion input";
        return as_result;
    }
    
    as_result.type = as->type;
    as_result.expected = as->expected;

    if (as->type == ASSERT_STATUS_EQ){    
        if (as->expected.type != VALUE_INT){
            as_result.passed = false;
            as_result.message = "ValueError on expected";
            return as_result; 
        }

        as_result.actual.type = VALUE_INT;
        as_result.actual.as.int_value = resp->status_code;

        as_result.passed = check_equal(as_result.actual, as_result.expected);

        as_result.message = as_result.passed
            ? "Status assertion passed"
            : "Status assertion failed";

        return as_result;
    }

    as_result.message = "Unknown assertion type";

    return as_result;
}