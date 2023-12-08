#ifndef clox_chunk_hpp
#define clox_chunk_hpp

#include "common.hpp"
#include "value.hpp"
// #include <cstdint>

typedef enum { // each instruction has an opcode
    OP_CONSTANT,
    OP_NIL,           // added in ch18
    OP_TRUE,          // added in ch18
    OP_FALSE,         // added in ch18
    OP_POP,           // added in ch21
    OP_GET_LOCAL,     // added in ch22
    OP_SET_LOCAL,     // added in ch22
    OP_GET_GLOBAL,    // added in ch21
    OP_DEFINE_GLOBAL, // added in ch21
    OP_SET_GLOBAL,    // added in ch21
    OP_EQUAL,         // added in ch18
    OP_GREATER,       // added in ch18
    OP_LESS,          // added in ch18
    OP_ADD,           // added in ch15
    OP_SUBTRACT,      // added in ch15
    OP_MULTIPLY,      // added in ch15
    OP_DIVIDE,        // added in ch15
    OP_NOT,           // added in ch18
    OP_NEGATE,        // added in ch15
    OP_PRINT,         // added in ch21
    OP_JUMP,          // added in ch23
    OP_JUMP_IF_FALSE, // added in ch23
    OP_LOOP,          // added in ch23
    OP_CALL,          // added in ch24
    OP_RETURN,
} OpCode;

typedef struct {
    int        count;
    int        capacity;
    uint8_t*   code;
    int*       lines;
    ValueArray constants;
} Chunk;

void initChunk (Chunk* chunk);
void freeChunk (Chunk* chunk);
// void writeChunk (Chunk* chunk, uint8_t byte);
void writeChunk (Chunk* chunk, uint8_t byte, int line);
int addConstant (Chunk* chunk, Value value);

#endif 