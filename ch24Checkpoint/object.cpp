#include <iostream>
#include <stdio.h>
#include <string.h>

#include "memory.hpp"
#include "object.hpp"
#include "table.hpp" // added in ch20
#include "value.hpp"
#include "vm.hpp"

#define ALLOCATE_OBJ(type, objectType) (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject (size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

ObjFunction* newFunction () { // added in ch24
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

ObjNative* newNative (NativeFn function) { // added in ch24
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

static ObjString* allocateString (char* chars, int length, uint32_t hash) { // added in ch20
// static ObjString* allocateString (char* chars, int length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash; // added in ch20
    tableSet(&vm.strings, string, NIL_VAL); // added in ch20
    return string;
}

static uint32_t hashString (const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* takeString (char* chars, int length) { 
    // return allocateString(chars, length); 
    uint32_t hash = hashString(chars, length); // added in ch20
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash); // added in ch20
    if (interned != NULL) { // added in ch20
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return allocateString(chars, length, hash); // added in ch20 
} // added in ch19

ObjString* copyString (const char* chars, int length) {
    uint32_t hash = hashString(chars, length); // added in ch20
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash); // added in ch20
    if (interned != NULL) return interned; // added in ch20

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    // return allocateString(heapChars, length);
    return allocateString(heapChars, length, hash); // added in ch20
}

static void printFunction (ObjFunction* function) { // added in ch24
  // printf("<fn %s>", function->name->chars);
    if (function->name == NULL) {
        // printf("<script>");
        std::cout << "<script>";
        return;
    }

    std::cout << "<fn " << function->name->chars << ">"; 
}

void printObject (Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_FUNCTION: // added in ch24
            printFunction(AS_FUNCTION(value));
            break;

        case OBJ_NATIVE: // added in ch24
            // printf("<native fn>");
            std::cout << "<native fn>";
            break;

        case OBJ_STRING:
            // printf("%s", AS_CSTRING(value));
            std::cout << AS_CSTRING(value);
            break;
    }
}