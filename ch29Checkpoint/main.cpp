#include <cstdlib> // includes added in ch16
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string_view> 

#include "common.hpp"
#include "chunk.hpp"
#include "debug.hpp"
#include "vm.hpp" // added in ch15 

static void repl () { // added in ch16
    char line[1024];
    // for (;;) {
    while (true) {
        // printf("> ");
        std::cout << "clox> ";

        if (!fgets(line, sizeof(line), stdin)) {
            //printf("\n");
            std::cout << "\n";
            break;
        }

        interpret(line);
    }
}

std::string readFile(std::string_view path) { // added in ch16
    std::ifstream file(path.data(), std::ios::binary);

    if (!file.is_open()) {
        std::cout << "Could not open file \"" << path << "\"." << std::endl;
        exit(74);
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer(fileSize, '\0');

    if (buffer.size() != fileSize) {
        std::cout << "Not enough memory to read \"" << path << "\"." << std::endl;
        exit(74);
    }

    file.read(buffer.data(), fileSize);  // Use buffer.data() instead of &buffer[0]

    file.close();
    return buffer;
}

/*
    static char* readFile(const char* path) {
        FILE* file = fopen(path, "rb");
        if (file == NULL) {
            fprintf(stderr, "Could not open file \"%s\".\n", path);
            exit(74);
        }

        fseek(file, 0L, SEEK_END);
        size_t fileSize = ftell(file);
        rewind(file);

        char* buffer = (char*)malloc(fileSize + 1);
        if (buffer == NULL) {
            fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
            exit(74);
        }

        size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
        if (bytesRead < fileSize) {
            fprintf(stderr, "Could not read file \"%s\".\n", path);
            exit(74);
        }

        buffer[bytesRead] = '\0';

        fclose(file);
        return buffer;
    }
*/

static void runFile (std::string_view path) { // added in ch16
    std::string src = readFile(path);
    InterpretResult result = interpret(src.data());

    if (result == INTERPRET_COMPILE_ERROR) exit(65);  
    if (result == INTERPRET_RUNTIME_ERROR) exit(70); 
}

/*
    static void runFile(const char* path) {
        char* source = readFile(path);
        InterpretResult result = interpret(source);
        free(source); 

        if (result == INTERPRET_COMPILE_ERROR) exit(65);
        if (result == INTERPRET_RUNTIME_ERROR) exit(70);
    }
*/

int main (int argc, char* argv[]) { // modified in ch16
    initVM(); // added in ch15

    if (argc == 1) repl();
    else if (argc == 2) runFile(argv[1]);
    else {
        std::cerr << "Usage: clox [path]\n";
        std::exit(64);
        // fprintf(stderr, "Usage: clox [path]\n");
        // exit(64);
    }

    return 0;

    // Chunk chunk;
    // initChunk(&chunk);

    // int constant = addConstant(&chunk, 1.2);
    // // writeChunk(&chunk, OP_CONSTANT);
    // // writeChunk(&chunk, constant);
    // // writeChunk(&chunk, OP_RETURN);

    // writeChunk(&chunk, OP_CONSTANT, 123);
    // writeChunk(&chunk, constant, 123);

    // constant = addConstant(&chunk, 3.4); // block added in ch15
    // writeChunk(&chunk, OP_CONSTANT, 123);
    // writeChunk(&chunk, constant, 123);

    // writeChunk(&chunk, OP_ADD, 123); // added in ch15

    // constant = addConstant(&chunk, 5.6); // block added in ch15
    // writeChunk(&chunk, OP_CONSTANT, 123);
    // writeChunk(&chunk, constant, 123);

    // writeChunk(&chunk, OP_DIVIDE, 123); // added in ch15

    // writeChunk(&chunk, OP_NEGATE, 123); // added in ch15    

    // writeChunk(&chunk, OP_RETURN, 123);

    // disassembleChunk(&chunk, "test chunk");
    // interpret(&chunk); // added in ch15
    // freeVM(); // added in ch15
    // freeChunk(&chunk);
}