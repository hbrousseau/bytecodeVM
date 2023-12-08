#include <stdio.h>
#include <iomanip>
#include <iostream>

#include "formatting.hpp"
#include "memory.hpp"
#include "value.hpp"

void initValueArray (ValueArray* array) {
    array->count = 0;
    array->capacity = 0;
    array->values = nullptr;
}

void writeValueArray (ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray (ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}   

void printValue (Value value) { // updated in ch18
    switch (value.type) {
        case VAL_BOOL:
        printf(AS_BOOL(value) ? "true" : "false");
        break;
        case VAL_NIL: printf("nil"); break;
        case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
    }

    // std::ostringstream oss;
    // oss << std::fixed << std::setprecision(6) << AS_NUMBER(value); // added in ch18
    // std::cout << trimTrailingZeros(oss.str());
    // // printf("%g", value); 
    // // std::cout << std::fixed << std::setprecision(6) << value;
}

bool valuesEqual(Value a, Value b) { // added in ch18
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:    return true;
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        default:         return false; // Unreachable.
    }
}
