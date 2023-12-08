#ifndef clox_memory_hpp
#define clox_memory_hpp

#include "common.hpp"
#include "object.hpp" // added in ch19

#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type) * (count)) // added in ch19

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0) // added in ch19

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
        (type*)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0)

void* reallocate (void* pointer, size_t oldSize, size_t newSize);
void  markValue(Value value);  // added in ch26
void  markObject(Obj* object); // added in ch26
void  collectGarbage();        // added in ch26
void  freeObjects();           // added in ch19

#endif