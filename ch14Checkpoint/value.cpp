#include <stdio.h>

#include "formatting.hpp"
#include "memory.hpp"
#include "value.hpp"

void initValueArray(ValueArray* array) {
    array->values = nullptr;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

void printValue(Value value) {
    // printf("%g", value);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << value;
    std::cout << trimTrailingZeros(oss.str());
}   