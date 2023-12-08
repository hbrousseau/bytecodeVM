#include "common.hpp"
#include "chunk.hpp"
#include "debug.hpp"
#include "vm.hpp" // added in ch15 

int main (int argc, char* argv[]) {
    initVM(); // added in ch15

    Chunk chunk;
    initChunk(&chunk);

    int constant = addConstant(&chunk, 1.2);
    // writeChunk(&chunk, OP_CONSTANT);
    // writeChunk(&chunk, constant);
    // writeChunk(&chunk, OP_RETURN);

    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    constant = addConstant(&chunk, 3.4); // block added in ch15
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    writeChunk(&chunk, OP_ADD, 123); // added in ch15

    constant = addConstant(&chunk, 5.6); // block added in ch15
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    writeChunk(&chunk, OP_DIVIDE, 123); // added in ch15

    writeChunk(&chunk, OP_NEGATE, 123); // added in ch15    

    writeChunk(&chunk, OP_RETURN, 123);

    disassembleChunk(&chunk, "test chunk");
    interpret(&chunk); // added in ch15
    freeVM(); // added in ch15
    freeChunk(&chunk);

    return 0;
}