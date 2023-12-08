#ifndef clox_value_hpp
#define clox_value_hpp

#include "common.hpp"

typedef struct Obj Obj; // added in ch19
typedef struct ObjString ObjString; // added in ch19

typedef enum { // added in ch18 
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ // added in ch19
} ValueType;

// typedef double Value;
typedef struct { // added in ch18
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

#define IS_BOOL(value)   ((value).type == VAL_BOOL) // added in ch18
#define IS_NIL(value)    ((value).type == VAL_NIL) // added in ch18
#define IS_NUMBER(value) ((value).type == VAL_NUMBER) // added in ch18
#define IS_OBJ(value)    ((value).type == VAL_OBJ) // added in ch19

#define AS_OBJ(value)     ((value).as.obj) // added in ch19
#define AS_BOOL(value)   ((value).as.boolean) // added in ch18
#define AS_NUMBER(value) ((value).as.number) // added in ch18

#define BOOL_VAL(value)   ((Value){ VAL_BOOL, { .boolean = value } }) // added the next two lines in ch18
#define NIL_VAL           ((Value){ VAL_NIL, { .number = 0 } })
#define NUMBER_VAL(value) ((Value){ VAL_NUMBER, { .number = value } })
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}}) // added in ch19

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