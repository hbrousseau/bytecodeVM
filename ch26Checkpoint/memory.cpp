#include <stdlib.h>

#include "compiler.hpp" // added in ch26
#include "memory.hpp"
#include "vm.hpp" // added in ch19

#ifdef DEBUG_LOG_GC // added in ch26
    #include <stdio.h>
    #include "debug.hpp"
#endif

#define GC_HEAP_GROW_FACTOR 2 // added in ch26

void* reallocate (void* pointer, size_t oldSize, size_t newSize) {
    vm.bytesAllocated += newSize - oldSize; // added in ch26
    if (newSize > oldSize) { // added in ch26
        #ifdef DEBUG_STRESS_GC
            collectGarbage();
        #endif
    }

    if (vm.bytesAllocated > vm.nextGC) { collectGarbage(); } // added in ch26

    if (newSize == 0) {
        free(pointer);
        return NULL;
    }
    
    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}

void markObject (Obj* object) { // added in ch26
    if (object == NULL)   return;
    if (object->isMarked) return;

    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

    #ifdef DEBUG_LOG_GC
        printf("%p mark ", (void*)object);
        printValue(OBJ_VAL(object));
        printf("\n");
    #endif

    object->isMarked = true;

    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Obj**)realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
        if (vm.grayStack == NULL) exit(1);
    }

    vm.grayStack[vm.grayCount++] = object;
}

void markValue (Value value) { if (IS_OBJ(value)) markObject(AS_OBJ(value)); } // added in ch26

static void markArray(ValueArray* array) { // added in ch26
    for (int i = 0; i < array->count; i++) { markValue(array->values[i]); }
}

static void blackenObject (Obj* object) { // added in ch26
    #ifdef DEBUG_LOG_GC
        printf("%p blacken ", (void*)object);
        printValue(OBJ_VAL(object));
        printf("\n");
    #endif

    switch (object->type) {
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            markObject((Obj*)closure->function);
            for (int i = 0; i < closure->upvalueCount; i++) { markObject((Obj*)closure->upvalues[i]); }
            break;
        }
        
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            markObject((Obj*)function->name);
            markArray(&function->chunk.constants);
            break;
        }

        case OBJ_UPVALUE:
            markValue(((ObjUpvalue*)object)->closed);
            break;

        case OBJ_NATIVE:
        case OBJ_STRING:
        break;
    }
}

static void freeObject (Obj* object) { // added in ch19
    #ifdef DEBUG_LOG_GC // added in ch26
        printf("%p free type %d\n", (void*)object, object->type);
    #endif

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
 
static void markRoots () { // added in ch26
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) { markValue(*slot); }

    for (int i = 0; i < vm.frameCount; i++) { markObject((Obj*)vm.frames[i].closure); }

    for (ObjUpvalue* upvalue = vm.openUpvalues; upvalue != NULL; upvalue = upvalue->next) { markObject((Obj*)upvalue); }


    markTable(&vm.globals);
    markCompilerRoots();
}

static void traceReferences () { // added in ch26
    while (vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }
}

static void sweep() { // added in ch26
    Obj* previous = NULL;
    Obj* object = vm.objects;
    while (object != NULL) {
        if (object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
        } 
        else {
            Obj* unreached = object;
            object = object->next;
                if (previous != NULL) { previous->next = object; }
                else { vm.objects = object; } 

                freeObject(unreached);
        }
    }
}

void collectGarbage () { // added in ch26
    #ifdef DEBUG_LOG_GC
        printf("-- gc begin\n");
        size_t before = vm.bytesAllocated;
    #endif

    markRoots();
    traceReferences();
    tableRemoveWhite(&vm.strings);
    sweep();

    #ifdef DEBUG_LOG_GC
        printf("-- gc end\n");
        printf("   collected %zu bytes (from %zu to %zu) next at %zu\n", before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC);
    #endif
}

void freeObjects () { // added in ch19
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }

    free(vm.grayStack); // added in ch26
}