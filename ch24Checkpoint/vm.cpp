#include <iostream>
#include <stdarg.h>     // added in ch18
#include <string.h>     // added in ch19
#include <time.h>       // added in ch24

#include "common.hpp"
#include "compiler.hpp" // added in ch16
#include "debug.hpp"
#include "object.hpp"   // added in ch19
#include "memory.hpp"   // added in ch19
#include "vm.hpp"

VM vm; 

static Value clockNative (int argCount, Value* args) { return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC); } // added in ch24

static void resetStack () { 
    vm.stackTop = vm.stack; 
    vm.frameCount = 0; // added in ch24
} 

static void runtimeError (const char* format, ...) { // added in ch18
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) { // added in ch24
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) fprintf(stderr, "script\n");
        else fprintf(stderr, "%s()\n", function->name->chars);
    }

    resetStack();

    // // size_t instruction = vm.ip - vm.chunk->code - 1;
    // // int line = vm.chunk->lines[instruction];

    // CallFrame* frame = &vm.frames[vm.frameCount - 1];                 // added in ch24
    // size_t instruction = frame->ip - frame->function->chunk.code - 1; // added in ch24 
    // int line = frame->function->chunk.lines[instruction];             // added in ch24

    // fprintf(stderr, "[line %d] in script\n", line);
}

static void defineNative (const char* name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM () {
    resetStack();
    vm.objects = NULL;                  // added in ch19
    initTable(&vm.globals);             // added in ch21
    initTable(&vm.strings);             // added in ch20
    defineNative("clock", clockNative); // added in ch24
}

void freeVM () { 
    freeTable(&vm.globals); // added in ch21
    freeTable(&vm.strings); // added in ch20
    freeObjects(); 
} // updated in ch21

void push (Value value) {
    *vm.stackTop = value;
     vm.stackTop++;
}

Value pop () {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek    (int distance) { return vm.stackTop[-1 - distance]; } // added in ch18

static bool call (ObjFunction* function, int argCount) { // added in ch24
    if (argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d.", function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue (Value callee, int argCount) { // added in ch24
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
        case OBJ_FUNCTION: 
            return call(AS_FUNCTION(callee), argCount);

        case OBJ_NATIVE: {
            NativeFn native = AS_NATIVE(callee);
            Value result = native(argCount, vm.stackTop - argCount);
            vm.stackTop -= argCount + 1;
            push(result);
            return true;
        }

        default:
            break; // Non-callable object type.
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

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
    // #define READ_BYTE() (*vm.ip++)
    // #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    // #define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1])) // added in ch23

    CallFrame* frame = &vm.frames[vm.frameCount - 1];                                       // added in ch24
    #define READ_BYTE() (*frame->ip++)                                                      // added in ch24
    #define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1])) // added in ch24
    #define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])          // added in ch24

    #define READ_STRING() AS_STRING(READ_CONSTANT()) // added in ch21
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

    while (1) {
        #ifdef DEBUG_TRACE_EXECUTION
            // print stack contents
            printf("          ");
            for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                printf("[ ");
                printValue(*slot);
                printf(" ]");
            }   
            printf("\n");
            disassembleInstruction(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code)); // added in ch24
            // disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
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
            case OP_POP: pop();                   break;       // added in ch21

            case OP_GET_LOCAL: {                               // added in ch22
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);                      // added in ch24
                // push(vm.stack[slot]); 
                break;
            }

            case OP_SET_LOCAL: {                               // added in ch22
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);                  // added in ch24
                // vm.stack[slot] = peek(0);
                break;
            }

            case OP_GET_GLOBAL: {                              // added in ch21
                ObjString* name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }

            case OP_DEFINE_GLOBAL: {                           // added in ch21
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }

            case OP_SET_GLOBAL: {                              // added in ch21
                ObjString* name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name); 
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

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
 
            case OP_PRINT: { // added in ch21
                printValue(pop());
                printf("\n"); 
                break;
            }

            case OP_JUMP: { // added in ch23
                uint16_t offset = READ_SHORT();
                frame->ip += offset;  // added in ch24  
                // vm.ip += offset;
                break;
            }

            case OP_JUMP_IF_FALSE: { // added in ch23
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) frame->ip += offset; // added in ch24
                // if (isFalsey(peek(0))) vm.ip += offset;
                break;
            }

            case OP_LOOP: { // added in ch23
                uint16_t offset = READ_SHORT();
                frame->ip -= offset; // added in ch24
                // vm.ip -= offset;
                break;
            }

            case OP_CALL: { // added in ch24
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) return INTERPRET_RUNTIME_ERROR; 
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }

            case OP_RETURN: { // modified in ch24
                Value result = pop();
                vm.frameCount--;
                if (vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;

                // // printValue(pop());
                // // std::cout << "\n";
                // return INTERPRET_OK;
            }
        }
    }

    #undef READ_BYTE
    #undef READ_SHORT // added in ch23
    #undef READ_CONSTANT
    #undef READ_STRING // added in ch21
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
    ObjFunction* function = compile(source); // added in ch24
    if (function == NULL) return INTERPRET_COMPILE_ERROR; // added in ch24 

    push(OBJ_VAL(function));                        // added in ch24 
    call(function, 0);                              // added in ch24

    // CallFrame* frame = &vm.frames[vm.frameCount++]; // added in ch24 
    // frame->function = function;                     // added in ch24 
    // frame->ip = function->chunk.code;               // added in ch24 
    // frame->slots = vm.stack;                        // added in ch24 
     
    return run(); // added in ch24

    // InterpretResult result = run();

    // freeChunk(&chunk);
    // return result;
    // Chunk chunk;
    // initChunk(&chunk);

    // if (!compile(source, &chunk)) {
    //     freeChunk(&chunk);
    //     return INTERPRET_COMPILE_ERROR;
    // }

    // vm.chunk = &chunk;
    // vm.ip = vm.chunk->code;
}