#ifndef clox_value_hpp
#define clox_value_hpp

#include <string.h> // added in ch30

#include "common.hpp"

typedef struct Obj Obj; // added in ch19
typedef struct ObjString ObjString; // added in ch19

#ifdef NAN_BOXING // added in ch30
    #define SIGN_BIT ((uint64_t)0x8000000000000000)
    #define QNAN     ((uint64_t)0x7ffc000000000000) // quiet NaN

    #define TAG_NIL   1 
    #define TAG_FALSE 2 
    #define TAG_TRUE  3

    typedef uint64_t Value;

    #define IS_BOOL(value)      (((value) | 1) == TRUE_VAL)
    #define IS_NIL(value)       ((value) == NIL_VAL)
    #define IS_NUMBER(value)    (((value) & QNAN) != QNAN)
    #define IS_OBJ(value)       (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

    #define AS_BOOL(value)      ((value) == TRUE_VAL)
    #define AS_NUMBER(value)    valueToNum(value)
    #define AS_OBJ(value)       ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

    #define BOOL_VAL(b)         ((b) ? TRUE_VAL : FALSE_VAL)
    #define FALSE_VAL           ((Value)(uint64_t)(QNAN | TAG_FALSE))
    #define TRUE_VAL            ((Value)(uint64_t)(QNAN | TAG_TRUE))    
    #define NIL_VAL             ((Value)(uint64_t)(QNAN | TAG_NIL))
    #define NUMBER_VAL(num)     numToValue(num)
    #define OBJ_VAL(obj)        (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

    // converts a value to a number
    static inline double valueToNum (Value value) {
        double num;
        memcpy(&num, &value, sizeof(Value));
        return num;
    }

    // converts a number to a value
    static inline Value numToValue(double num) {
        Value value;
        memcpy(&value, &num, sizeof(double));
        return value;
    }

#else

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

#endif // end of ch30

// represents a dynamic array of values
typedef struct {
    int    capacity;
    int    count;
    Value* values;
} ValueArray;

bool valuesEqual     (Value a, Value b); // added in ch18
void initValueArray  (ValueArray* array);
void writeValueArray (ValueArray* array, Value value);  
void freeValueArray  (ValueArray* array);  
void printValue      (Value value);
void push            (Value value);
Value pop            ();


#endif