#include <iomanip>
#include <iostream>
#include <stdio.h>

#include "debug.hpp"
#include "value.hpp"

void disassembleChunk(Chunk* chunk, const char* name) {
    // printf("== %s ==\n", name);
    std::cout << "== " << name << " ==" << '\n';

    for (int offset = 0; offset < chunk->count;) { offset = disassembleInstruction(chunk, offset); }
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    // std::cout << std::setw(16) << std::setfill(' ') << std::left << name << std::setw(4) << std::setfill(' ') << std::right << constant << " '"; // for some reason, this doesn't print the 0th constant
    printValue(chunk->constants.values[constant]);
    // printf("'\n");
    std::cout << "'" << '\n';
    return offset + 2;
}

static int simpleInstruction(const char* name, int offset) {
    // printf("%s\n", name);
    std::cout << name << '\n';
    return offset + 1;
}

int disassembleInstruction(Chunk* chunk, int offset) {
    // printf("%04d ", offset);
    std::cout << std::setw(4) << std::setfill('0') << offset << " ";
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        // printf("   | ");
        std::cout << "   | ";
    } 
    else {
        // printf("%4d ", chunk->lines[offset]);
        std::cout << std::setw(4) << std::setfill(' ') << chunk->lines[offset] << " ";
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);

        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);

        default:
            // printf("Unknown opcode %d\n", instruction);
            std::cout << "Unknown opcode " << instruction << '\n';
        return offset + 1;
    }
}
