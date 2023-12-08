#ifndef clox_compiler_hpp
#define clox_compiler_hpp

#include "vm.hpp"

ObjFunction* compile (const char* source); // added in ch24

// bool compile (const char* source, Chunk* chunk);

// void compile (const char* source);

#endif