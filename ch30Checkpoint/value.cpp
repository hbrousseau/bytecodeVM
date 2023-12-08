#include <stdio.h>
#include <iomanip>
#include <iostream>
#include <string.h>

#include "formatting.hpp"
#include "memory.hpp"
#include "object.hpp" // added in ch19
#include "value.hpp"

// initializes the value array
void initValueArray (ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

// writes the value array
void writeValueArray (ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCap = array->capacity;
        array->capacity = GROW_CAPACITY(oldCap);
        array->values = GROW_ARRAY(Value, array->values, oldCap, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

// frees the value array
void freeValueArray (ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}   

// prints the value array
void printValue (Value value) { // updated in ch18
    #ifdef NAN_BOXING // added in ch30
        if (IS_BOOL(value))        { printf(AS_BOOL(value) ? "true" : "false"); } 
        else if (IS_NIL(value))    { printf("nil"); } 
        else if (IS_NUMBER(value)) { printf("%g", AS_NUMBER(value)); } 
        else if (IS_OBJ(value))    { printObject(value); }
    #else

    switch (value.type) {
        case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;

        case VAL_NIL:    printf("nil"); break;

        case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
        
        case VAL_OBJ:    printObject(value); break; // added in ch19
    }

    #endif // added in ch30

    // std::ostringstream oss;
    // oss << std::fixed << std::setprecision(6) << AS_NUMBER(value); // added in ch18
    // std::cout << trimTrailingZeros(oss.str());
    // // printf("%g", value); 
    // // std::cout << std::fixed << std::setprecision(6) << value;
}

// checks to see if two values are equal
bool valuesEqual (Value a, Value b) { // added in ch18
    #ifdef NAN_BOXING // added in ch30
        if (IS_NUMBER(a) && IS_NUMBER(b)) { return AS_NUMBER(a) == AS_NUMBER(b); }
        return a == b;
    #else

    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:    return true;
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b); // modified in ch20
        // case VAL_OBJ: {
        //     ObjString* aString = AS_STRING(a);
        //     ObjString* bString = AS_STRING(b);
        //     return aString->length == bString->length && memcmp(aString->chars, bString->chars, aString->length) == 0;
        // }
        default:         return false; // Unreachable.
    }

    #endif // added in ch30

}
