#include <iomanip>
#include <iostream>
#include <stdio.h>

#include "common.hpp"
#include "compiler.hpp"
#include "scanner.hpp"

void compile(const char* source) { 
    initScanner(source);
    int line = -1;
    // for (;;) {
    while (true) {
        Token token = scanToken();
        if (token.line != line) {
            // printf("%4d ", token.line);
            std::cout << std::setw(4) << std::setfill(' ') << token.line << " ";
            line = token.line;
        } 
        else {
            // printf("   | ");
            std::cout << "   | ";
        }
        // printf("%2d '%.*s'\n", token.type, token.length, token.start); 
        std::cout << std::setw(2) << std::setfill(' ') << token.type << " '" << std::setw(token.length) << std::setfill(' ') << token.start << "'\n";

        if (token.type == TOKEN_EOF) break;
    }
}