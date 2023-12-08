#include <stdlib.h>

#include "chunk.hpp"
#include "memory.hpp"
#include "value.hpp" // added in ch26

// initChunk is called when a new chunk is created
void initChunk (Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

// freeChunk is called when a chunk is destroyed
void freeChunk (Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

// writeChunk is called when a new instruction is added to the chunk
// void writeChunk (Chunk* chunk, uint8_t byte) {
void writeChunk (Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCap = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCap);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCap, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, oldCap, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

// addConstant is called when a new constant is added to the chunk
int addConstant(Chunk* chunk, Value value) {
    push(value); // added in ch26
    writeValueArray(&chunk->constants, value);
    pop();     // added in ch26
    return chunk->constants.count - 1;
}