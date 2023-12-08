#include <stdio.h>
#include <iostream>
#include <iomanip>

#include "debug.hpp"
#include "value.hpp"

void disassembleChunk (Chunk* chunk, const char* name) {
    // printf("== %s ==\n", name);
    std::cout << "== " << name << " ==" << "\n";

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int simpleInstruction (const char* name, int offset) {
    // printf("%s\n", name);
    std::cout << name << "\n";
    return offset + 1;
}

static int byteInstruction(const char* name, Chunk* chunk, int offset) { // added in ch22
    uint8_t slot = chunk->code[offset + 1];
    // printf("%-16s %4d\n", name, slot);
    std::cout << std::left << std::setw(16) << std::setfill(' ') << name << std::right << std::setw(4) << std::setfill(' ') << slot << "\n";
    return offset + 2; 
}

static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset) { // added in ch23
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    // printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    std::cout << std::left << std::setw(16) << std::setfill(' ') << name << std::right << std::setw(4) << std::setfill(' ') << offset << " -> " << offset + 3 + sign * jump << "\n";
    return offset + 3;
}

static int constantInstruction (const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    //std::cout << std::left << std::setw(16) << std::setfill(' ') << name << std::right << std::setw(4) << std::setfill(' ') << constant << " '"; // for some reason this doesn't work
    printValue(chunk->constants.values[constant]);
    // printf("'\n");
    std::cout << "'\n";

    return offset + 2;
}

int disassembleInstruction (Chunk* chunk, int offset) {
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

        case OP_NIL: // added in ch18
            return simpleInstruction("OP_NIL", offset);

        case OP_TRUE: // added in ch18
            return simpleInstruction("OP_TRUE", offset);

        case OP_FALSE: // added in ch18
            return simpleInstruction("OP_FALSE", offset);

        case OP_POP: // added in ch21
            return simpleInstruction("OP_POP", offset);

        case OP_GET_LOCAL: // added in ch22
            return byteInstruction("OP_GET_LOCAL", chunk, offset);

        case OP_SET_LOCAL: // added in ch22
            return byteInstruction("OP_SET_LOCAL", chunk, offset);

        case OP_GET_GLOBAL: // added in ch21
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);

        case OP_DEFINE_GLOBAL: // added in ch21
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);

        case OP_SET_GLOBAL: // added in ch21
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);

        case OP_EQUAL: // added in ch18
            return simpleInstruction("OP_EQUAL", offset);

        case OP_GREATER: // added in ch18
            return simpleInstruction("OP_GREATER", offset);

        case OP_LESS: // added in ch18
            return simpleInstruction("OP_LESS", offset);

        case OP_ADD: // added in ch15
            return simpleInstruction("OP_ADD", offset);

        case OP_SUBTRACT: // added in ch15
            return simpleInstruction("OP_SUBTRACT", offset);

        case OP_MULTIPLY: // added in ch15
            return simpleInstruction("OP_MULTIPLY", offset);

        case OP_DIVIDE: // added in ch15
            return simpleInstruction("OP_DIVIDE", offset);

        case OP_NOT: // added in ch18
            return simpleInstruction("OP_NOT", offset);

        case OP_NEGATE: // added in ch15
            return simpleInstruction("OP_NEGATE", offset);

        case OP_PRINT: // added in ch21
            return simpleInstruction("OP_PRINT", offset);

        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);

        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);

        case OP_LOOP: // added in ch23
            return jumpInstruction("OP_LOOP", -1, chunk, offset);

        case OP_RETURN:
            // printf("OP_RETURN\n");
            std::cout << "OP_RETURN\n";
            return offset + 1;

        default:
            // printf("Unknown opcode %d\n", instruction);
            std::cout << "Unknown opcode " << instruction << std::endl;
            return offset + 1;
    }
}

