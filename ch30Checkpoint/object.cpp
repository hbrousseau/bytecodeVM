#include <iostream>
#include <stdio.h>
#include <string.h>

#include "memory.hpp"
#include "object.hpp"
#include "table.hpp" // added in ch20
#include "value.hpp"
#include "vm.hpp"

#define ALLOCATE_OBJ(type, objectType) (type*)allocateObject(sizeof(type), objectType) // allocates memory for an object

// creates and allocates memory for an object
static Obj* allocateObject (size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false; // added in ch26
    object->next = vm.objects;
    vm.objects = object;

    #ifdef DEBUG_LOG_GC     // added in ch26
        printf("%p allocate %zu for %d\n", (void*)object, size, type);
    #endif

    return object;
}

// instantiates a new bound method
ObjBoundMethod* newBoundMethod (Value receiver, ObjClosure* method) { // added in ch28
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

// instantiates a new class
ObjClass* newClass (ObjString* name) { // added in ch27
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name; 
    initTable(&klass->methods); // added in ch28
    return klass;
}

// instantiates a new closure
ObjClosure* newClosure (ObjFunction* function) { // added in ch25
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) { upvalues[i] = NULL; }

    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

// instantiates a new function
ObjFunction* newFunction () { // added in ch24
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0; // added in ch25
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

// instantiates a new instance
ObjInstance* newInstance (ObjClass* klass) { // added in ch27
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
}

// instantiates a new native function
ObjNative* newNative (NativeFn function) { // added in ch24
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

// allocates memory for a string
static ObjString* allocateString (char* chars, int length, uint32_t hash) { // added in ch20
// static ObjString* allocateString (char* chars, int length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash; // added in ch20

    push(OBJ_VAL(string)); // added in ch26
    tableSet(&vm.strings, string, NIL_VAL); // added in ch20
    pop(); // added in ch26

    return string;
}

// hash string is called when a string is created
static uint32_t hashString (const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

// takes ownership of a string
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

// copies a string and creates an ObjString
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

// instantiates a new upvalue
ObjUpvalue* newUpvalue (Value* slot) { // added in ch25
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed     = NIL_VAL;
    upvalue->location   = slot;
    upvalue->next       = NULL;
    return upvalue;
}

// prints function in a human readable format
static void printFunction (ObjFunction* function) { // added in ch24
  // printf("<fn %s>", function->name->chars);
    if (function->name == NULL) {
        // printf("<script>");
        std::cout << "<script>";
        return;
    }

    std::cout << "<fn " << function->name->chars << ">"; 
}

// prints object in a human readable format
void printObject (Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD: // added in ch28
            printFunction(AS_BOUND_METHOD(value)->method->function);
            break;

        case OBJ_CLASS: // added in ch27
            printf("%s", AS_CLASS(value)->name->chars);
            break;

        case OBJ_CLOSURE: // added in ch25
            printFunction(AS_CLOSURE(value)->function);
            break;

        case OBJ_FUNCTION: // added in ch24
            printFunction(AS_FUNCTION(value));
            break;

        case OBJ_INSTANCE:
            printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
            break;

        case OBJ_NATIVE: // added in ch24
            // printf("<native fn>");
            std::cout << "<native fn>";
            break;

        case OBJ_STRING:
            // printf("%s", AS_CSTRING(value));
            std::cout << AS_CSTRING(value);
            break;

        case OBJ_UPVALUE: // added in ch25
            printf("upvalue");
            break;
    }
}