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

void printValue (Value value) {
    // printf("%g", value); 
    // std::cout << std::fixed << std::setprecision(6) << value;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << value;
    std::cout << trimTrailingZeros(oss.str());
}