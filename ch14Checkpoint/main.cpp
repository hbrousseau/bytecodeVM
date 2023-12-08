#include "common.hpp"
#include "chunk.hpp"
#include "debug.hpp"

int main(int argc, const char* argv[]) {
    Chunk chunk;
    initChunk(&chunk);

    int constant = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    writeChunk(&chunk, OP_RETURN, 123);

    disassembleChunk(&chunk, "test chunk");

    freeChunk(&chunk);
    return 0;
    // writeChunk(&chunk, OP_CONSTANT);
    // writeChunk(&chunk, constant);

    // writeChunk(&chunk, OP_RETURN);
}