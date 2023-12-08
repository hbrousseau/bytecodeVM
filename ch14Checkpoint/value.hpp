#ifndef clox_value_hpp
#define clox_value_hpp

#include "common.hpp"

typedef double Value;

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

void initValueArray  (ValueArray* array);
void writeValueArray (ValueArray* array, Value value);
void freeValueArray  (ValueArray* array);
void printValue      (Value value);

#endif