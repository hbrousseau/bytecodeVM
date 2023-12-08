#include <iostream>
#include <stdarg.h>     // added in ch18
#include <string.h>     // added in ch19

#include "common.hpp"
#include "compiler.hpp" // added in ch16
#include "debug.hpp"
#include "object.hpp"   // added in ch19
#include "memory.hpp"   // added in ch19
#include "vm.hpp"

VM vm; 

static void resetStack () { vm.stackTop = vm.stack; } 

static void runtimeError (const char* format, ...) { // added in ch18
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM () {
    resetStack();
    vm.objects = nullptr;   // added in ch19
    initTable(&vm.strings); // added in ch20
}

void freeVM () { 
    freeTable(&vm.strings); // added in ch20
    freeObjects(); 
} // updated in ch20

void push (Value value) {
    *vm.stackTop = value;
     vm.stackTop++;
}

Value pop () {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek    (int distance) { return vm.stackTop[-1 - distance]; } // added in ch18
static bool isFalsey (Value value) { return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value)); } // added in ch18

static void concatenate () { // added in ch19
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    push(OBJ_VAL(result));
}

static InterpretResult run () {
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    #define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while (false)

    // #define BINARY_OP(op) \
    //     do { \
    //         double b = pop(); \
    //         double a = pop(); \
    //         push(a op b); \
    //     } while (false)

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

            case OP_NIL: push(NIL_VAL);           break;       // added in ch18
            case OP_TRUE: push(BOOL_VAL(true));   break;       // added in ch18 
            case OP_FALSE: push(BOOL_VAL(false)); break;       // added in ch18

            case OP_EQUAL: {                                   // added in ch18
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }

            case OP_GREATER:  BINARY_OP(BOOL_VAL, >);   break; // added in ch18
            case OP_LESS:     BINARY_OP(BOOL_VAL, <);   break; // added in ch18

            // case OP_ADD:      BINARY_OP(NUMBER_VAL, +); break; // updated in ch18
            case OP_ADD: { // updated in ch19
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } 
                else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                }
                else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break; // updated in ch18
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break; // updated in ch18
            case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break; // updated in ch18

            case OP_NOT: // added in ch18
                push(BOOL_VAL(isFalsey(pop())));
                break;

            // case OP_ADD: {
            //     BINARY_OP(+);
            //     break;
            // }

            // case OP_SUBTRACT: {
            //     BINARY_OP(-);
            //     break;
            // }

            // case OP_MULTIPLY: {
            //     BINARY_OP(*);
            //     break;
            // }

            // case OP_DIVIDE: {
            //     BINARY_OP(/);
            //     break;
            // }
 
            case OP_NEGATE: // updated in ch18
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;

            // case OP_NEGATE: {
            //     push(-pop());
            //     break;
            // }

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

// InterpretResult interpret (Chunk* chunk) { // modified in ch16
//     vm.chunk = chunk; 
//     vm.ip = vm.chunk->code;
//     return run();
// }

// InterpretResult interpret(const char* source) { // added in ch16 ... modified in ch17
//     compile(source);
//     return INTERPRET_OK;
// }

InterpretResult interpret (const char* source) { // added in ch17
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}