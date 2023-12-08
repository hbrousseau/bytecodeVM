#include <stdlib.h>

#include "memory.hpp"
#include "vm.hpp" // added in ch19

void* reallocate (void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return nullptr;
    }
    
    void* result = realloc(pointer, newSize);
    if (result == nullptr) exit(1);
    return result;
}

static void freeObject (Obj* object) { // added in ch19
    switch (object->type) {
        case OBJ_STRING: {
        ObjString* string = (ObjString*)object;
        FREE_ARRAY(char, string->chars, string->length + 1);
        FREE(ObjString, object);
        break;
        }
    }
}

void freeObjects () { // added in ch19
    Obj* object = vm.objects;
    while (object != nullptr) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}