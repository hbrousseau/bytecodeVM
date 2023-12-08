#ifndef clox_memory_hpp
#define clox_memory_hpp

#include "common.hpp"
#include "object.hpp" // added in ch19

#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type) * (count)) // added in ch19... this is the function that allocates memory

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0) // added in ch19... this is the function that frees memory

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)// this is the function that grows the capacity of the heap

#define GROW_ARRAY(type, pointer, oldCount, newCount) (type*)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount)) // grows the array

#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0) // frees the array

void* reallocate (void* pointer, size_t oldSize, size_t newSize);
void  markValue(Value value);  // added in ch26
void  markObject(Obj* object); // added in ch26
void  collectGarbage();        // added in ch26
void  freeObjects();           // added in ch19

#endif