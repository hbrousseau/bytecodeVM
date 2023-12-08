#include <stdlib.h>

#include "memory.hpp"
#include "vm.hpp" // added in ch19

void* reallocate (void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }
    
    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}

static void freeObject (Obj* object) { // added in ch19
    switch (object->type) {
        case OBJ_CLOSURE: { // added in ch25
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(ObjClosure, object);
            break;
        }

        case OBJ_FUNCTION: { // added in ch24
            ObjFunction* function = (ObjFunction*)object;
            freeChunk(&function->chunk);
            FREE(ObjFunction, object);
            break;
        }

        case OBJ_NATIVE: // added in ch24
            FREE(ObjNative, object);
            break;

        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }

        case OBJ_UPVALUE: // added in ch25
            FREE(ObjUpvalue, object);
            break;
    }
}

void freeObjects () { // added in ch19
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}