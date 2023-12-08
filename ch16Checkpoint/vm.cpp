#include <iostream>

#include "common.hpp"
#include "compiler.hpp" // added in ch16
#include "debug.hpp"
#include "vm.hpp"

VM vm; 

static void resetStack () { vm.stackTop = vm.stack; }

void initVM () {
    resetStack();
}

void freeVM () {}

static InterpretResult run () {
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    #define BINARY_OP(op) \
        do { \
            double b = pop(); \
            double a = pop(); \
            push(a op b); \
        } while (false)

    while (true) {
        #ifdef DEBUG_TRACE_EXECUTION
            std::cout << "          ";
            for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                std::cout << "[ ";
                printValue(*slot);
                std::cout << " ]";
            }   
            std::cout << "\n";
            disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
        #endif

        uint8_t instruction;
        
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant  = READ_CONSTANT();
                // printValue(constant);
                // // printf("\n");
                // std::cout << "\n";
                push(constant);
                break;
            }

            case OP_ADD: {
                BINARY_OP(+);
                break;
            }

            case OP_SUBTRACT: {
                BINARY_OP(-);
                break;
            }

            case OP_MULTIPLY: {
                BINARY_OP(*);
                break;
            }

            case OP_DIVIDE: {
                BINARY_OP(/);
                break;
            }

            case OP_NEGATE: {
                push(-pop());
                break;
            }

            case OP_RETURN: {
                printValue(pop());
                std::cout << "\n";
                return INTERPRET_OK;
            }
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef BINARY_OP
}

void push (Value value) {
    *vm.stackTop = value;
     vm.stackTop++;
}

Value pop () {
    vm.stackTop--;
    return *vm.stackTop;
}

// InterpretResult interpret (Chunk* chunk) { // modified in ch16
//     vm.chunk = chunk; 
//     vm.ip = vm.chunk->code;
//     return run();
// }

InterpretResult interpret(const char* source) { // added in ch16
    compile(source);
    return INTERPRET_OK;
}