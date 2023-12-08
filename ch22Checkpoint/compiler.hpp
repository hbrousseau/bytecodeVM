#ifndef clox_compiler_hpp
#define clox_compiler_hpp

#include "vm.hpp"

bool compile (const char* source, Chunk* chunk);

// void compile (const char* source);

#endif