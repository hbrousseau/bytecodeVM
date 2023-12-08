#ifndef clox_debug_hpp
#define clox_debug_hpp

#include <iostream>

#include "chunk.hpp"

void disassembleChunk      (Chunk* chunk, const char* name);
int disassembleInstruction (Chunk* chunk, int offset);

#endif