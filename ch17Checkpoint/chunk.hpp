#ifndef clox_chunk_hpp
#define clox_chunk_hpp

#include "common.hpp"
#include "value.hpp"
// #include <cstdint>

typedef enum { // each instruction has an opcode
    OP_CONSTANT,
    OP_ADD,      // added in ch15
    OP_SUBTRACT, // added in ch15
    OP_MULTIPLY, // added in ch15
    OP_DIVIDE,   // added in ch15
    OP_NEGATE,   // added in ch15
    OP_RETURN,
} OpCode;

typedef struct { // hold the bytecode
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

void initChunk (Chunk* chunk);
void freeChunk (Chunk* chunk);
// void writeChunk (Chunk* chunk, uint8_t byte);
void writeChunk (Chunk* chunk, uint8_t byte, int line);
int addConstant (Chunk* chunk, Value value);

#endif 