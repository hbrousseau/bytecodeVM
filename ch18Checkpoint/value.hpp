#ifndef clox_value_hpp
#define clox_value_hpp

#include "common.hpp"

typedef enum { // added in ch18
    VAL_BOOL,
    VAL_NIL, 
    VAL_NUMBER,
} ValueType;

// typedef double Value;
typedef struct { // updated in ch18
    ValueType type;
    union {
        bool boolean;
        double number;
    } as; 
} Value;

#define IS_BOOL(value)    ((value).type == VAL_BOOL)               // added in ch18
#define IS_NIL(value)     ((value).type == VAL_NIL)                // added in ch18
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)             // added in ch18

#define AS_BOOL(value)    ((value).as.boolean)                     // added in ch18
#define AS_NUMBER(value)  ((value).as.number)                      // added in ch18 

#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})  // added in ch18
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})        // added in ch18
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}}) // added in ch18

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

bool valuesEqual     (Value a, Value b); // added in ch18
void initValueArray  (ValueArray* array);
void writeValueArray (ValueArray* array, Value value);  
void freeValueArray  (ValueArray* array);  
void printValue      (Value value);

#endif