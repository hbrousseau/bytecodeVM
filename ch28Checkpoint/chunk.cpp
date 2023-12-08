#include <stdlib.h>

#include "chunk.hpp"
#include "memory.hpp"
#include "value.hpp" // added in ch26

void initChunk (Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk (Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

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

int addConstant(Chunk* chunk, Value value) {
    push(value); // added in ch26
    writeValueArray(&chunk->constants, value);
    pop();     // added in ch26
    return chunk->constants.count - 1;
}