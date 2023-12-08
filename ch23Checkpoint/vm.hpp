#ifndef clox_vm_hpp
#define clox_vm_hpp

#include "chunk.hpp"
#include "table.hpp" // added in ch20
#include "value.hpp"
 
// #define STACK_MAX 256

typedef struct {
    Chunk*   chunk;
    uint8_t* ip;
    //   Value stack[STACK_MAX];
    Value    stack[256];
    Value*   stackTop;
    Table globals; // added in ch21
    Table strings; // added in ch20
    Obj*     objects; // added in ch19
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm; // added in ch19

void initVM ();
void freeVM ();
static InterpretResult run ();
// InterpretResult interpret (Chunk* chunk); // modified in ch16
InterpretResult interpret (const char* source); // added in ch16
void push (Value value);
Value pop ();

#endif