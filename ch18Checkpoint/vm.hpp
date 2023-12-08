#ifndef clox_vm_hpp
#define clox_vm_hpp

#include "chunk.hpp"
#include "value.hpp"

// #define STACK_MAX 256

typedef struct {
    Chunk*   chunk;
    uint8_t* ip;
    //   Value stack[STACK_MAX];
    Value    stack[256];
    Value*   stackTop;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM ();
void freeVM ();
static InterpretResult run ();
// InterpretResult interpret (Chunk* chunk); // modified in ch16
InterpretResult interpret (const char* source); // added in ch16
void push (Value value);
Value pop ();

#endif