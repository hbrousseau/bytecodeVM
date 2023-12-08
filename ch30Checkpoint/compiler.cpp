#include <iomanip>
#include <iostream>
#include <stdlib.h> // added in ch17
#include <string.h> // added in ch22

#include "common.hpp"
#include "compiler.hpp"
#include "object.hpp" // added in ch19
#include "memory.hpp" // added in ch26
#include "scanner.hpp"

#ifdef DEBUG_PRINT_CODE // added in ch17
#include "debug.hpp"
#endif

// parser struct is for tracking the state of the parser
typedef struct { // added in ch17
    Token current;
    Token previous;
    bool  hadError;
    bool  panicAtTheDisco;
} Parser;

// precedence table is for tracking the precedence of operators
typedef enum {  // added in ch17
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

// typedef void (*ParseFn)(); // added in ch17
typedef void (*ParseFn)(bool canAssign); // updated in ch21 ... parse functions now take a bool parameter that indicates whether or not the expression can be assigned to

// parse rule struct is for tracking the prefix and infix parse functions for each token type
typedef struct { // added in ch17... Pratt parser
    ParseFn    prefix;
    ParseFn    infix;
    Precedence precedence;
} ParseRule;

// local struct is for tracking local variables
typedef struct { // added in ch22
    Token name;
    int   depth;
    bool  isCaptured; // added in ch25
} Local;

// upvalue struct is for tracking variables from enclosing functions
typedef struct { // added in ch25
    uint8_t index;
    bool    isLocal;
} Upvalue;

// function type is for tracking whether the function is a script, function, or initializer
typedef enum { // added in ch24
    TYPE_FUNCTION,
    TYPE_INITIALIZER, // added in ch28
    TYPE_METHOD,      // added in ch28
    TYPE_SCRIPT
} FunctionType;

// compiler struct is for tracking the state of the compiler
// typedef struct { // added in ch22
typedef struct Compiler {          // modified in ch24
    struct Compiler* enclosing;    // added in ch24
    ObjFunction*     function;     // added in ch24
    FunctionType     type;         // added in ch24

    Local   locals[UINT8_COUNT];
    int     localCount;
    Upvalue upvalues[UINT8_COUNT]; // added in ch25
    int     scopeDepth;
} Compiler;

// class compiler struct is for tracking the state of the class compiler
typedef struct ClassCompiler { // added in ch28
    struct ClassCompiler* enclosing;
    bool                  hasSuperclass; // added in ch29
} ClassCompiler;

// define variables for tracking the state of the parser, compiler, and class compiler
Parser         parser;              // added in ch17
Compiler*      current = NULL;      // added in ch22
ClassCompiler* currClass = NULL; // added in ch28

// current chunk is for tracking the current chunk being compiled
static Chunk* currChunk () { return &current->function->chunk; } // added in ch24

// Compiler* current = NULL; // added in ch22
// Chunk* compilingChunk; // added in ch17
// static Chunk* currentChunk () { return compilingChunk; } // added in ch17

// error at is for reporting errors at a specific token
static void errorAt (Token* token, const char* message) { // added in ch17
    if (parser.panicAtTheDisco) return; // stop cascading errors
    parser.panicAtTheDisco = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) fprintf(stderr, " at end");
    else if (token->type == TOKEN_ERROR) { /*Nothing*/ }
    else fprintf(stderr, " at '%.*s'", token->length, token->start);

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error (const char* message) { errorAt(&parser.previous, message); } // added in ch17... report error at previous token
static void errorAtCurrent (const char* message) { errorAt(&parser.current, message); } // added in ch17... report error at current token

// advancing the parser to the next token
static void nextToken () { // added in ch17
    parser.previous = parser.current;

    while (1) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

// consume token is for consuming the current token if it matches the expected type
static void consumeToken (TokenType type, const char* message) { // added in ch17
    if (parser.current.type == type) {
        nextToken();
        return;
    }

    errorAtCurrent(message);
}

// checking the type of token
static bool checkType (TokenType type) { return parser.current.type == type; } // added in ch21

// consumes the next Token if it is of a matching TokenType.
static bool matchType (TokenType type) { // added in ch21   
    if (!checkType(type)) return false;
    nextToken();
    return true;
}

// emitting one or two bytes of bytecode
static void emitByte (uint8_t byte) { writeChunk(currChunk(), byte, parser.previous.line); } // added in ch17
static void emitTwoBytes (uint8_t byte1, uint8_t byte2) { emitByte(byte1); emitByte(byte2); }   // added in ch17

// appending loop to curr chunk
static void emitLoop (int loopStart) {                                                          // added in ch23
    emitByte(OP_LOOP);

    int offset = currChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body too large.");

    emitTwoBytes((offset >> 8) & 0xff, offset & 0xff);
}

// adds jump instruction to curr chunk
static int emitJump (uint8_t instruction) {                                                     // added in ch23
    emitByte(instruction);
    emitTwoBytes(0xff, 0xff);
    return currChunk()->count - 2;
}

static void emitReturn () {                                                                     // added in ch17
    // emitByte(OP_NIL); // added in ch24
    if (current->type == TYPE_INITIALIZER) { emitTwoBytes(OP_GET_LOCAL, 0); }                      // added in ch28
    else { emitByte(OP_NIL); }                                                                  // added in ch28

    emitByte(OP_RETURN); 
}                                              

// converts value to constant and adds it to the chunk
static uint8_t makeConstant (Value value) {                                                     // added in ch17
    int constant = addConstant(currChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

// emits a constant instruction
static void emitConstant (Value value) { emitTwoBytes(OP_CONSTANT, makeConstant(value)); }        // added in ch17

// patches the jump offset
static void patchJump (int offset) {                                                           // added in ch23
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = currChunk()->count - offset - 2;

    if (jump > UINT16_MAX) error("Too much code to jump over.");

    currChunk()->code[offset] = (jump >> 8) & 0xff;
    currChunk()->code[offset + 1] = jump & 0xff;
}

// initializes the compiler
// static void initCompiler (Compiler* compiler) {                                              // added in ch22 
static void initCompiler (Compiler* compiler, FunctionType type) {                              // modified in ch24
    compiler->enclosing = current;      // added in ch24
    compiler->function = NULL;          // added in ch24
    compiler->type = type;              // added in ch24
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction(); // added in ch24
    current = compiler;

    if (type != TYPE_SCRIPT) { current->function->name = copyString(parser.previous.start, parser.previous.length); }// added in ch24

    Local* local = &current->locals[current->localCount++]; // added in ch24
    local->depth = 0;                                       // added in ch24
    local->isCaptured = false;                              // added in ch25

    if (type != TYPE_FUNCTION) {                            // added in ch28
        local->name.start = "this";
        local->name.length = 4;
    } 
    else {                                                  // added in ch28
        local->name.start = "";
        local->name.length = 0;
    }

}

// ends curr compiler
// static void endCompiler () {                                                                   // added in ch17
static ObjFunction* endCompiler () {                                                           // modified in ch24
    emitReturn();
    ObjFunction* function = current->function; // added in ch24

    #ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        // disassembleChunk(currentChunk(), "code");
        disassembleChunk(currentChunk(), function->name != NULL ? function->name->chars : "<script>"); // added in ch24
    }
    #endif

    current = current->enclosing; // added in ch24
    return function;              // added in ch24
}

// begins a new scope
static void beginScope() { current->scopeDepth++; }                                            // added in ch22

// ends a scope
static void endScope ()   {                                                                    // added in ch22
    current->scopeDepth--;
    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        // emitByte(OP_POP);
        if (current->locals[current->localCount - 1].isCaptured) emitByte(OP_CLOSE_UPVALUE); // added in ch25
        else  emitByte(OP_POP);                                                              // added in ch25

        current->localCount--;
    }
}         

// forward declarations 
static void expression();                                                                      // added in ch17
static void statement();                                                                       // added in ch21
static void declaration();                                                                     // added in ch21
static ParseRule* getRule(TokenType type);                                                     // added in ch17
static void parsePrecedence(Precedence precedence);                                            // added in ch17
// static void and_(bool canAssign);                                                           // added in ch23

// makes an identifier constant
static uint8_t identifierConstant(Token* name) { return makeConstant(OBJ_VAL(copyString(name->start, name->length))); } // added in ch21

// determines if two ids are equal
static bool areIdentifiersEqual (Token* a, Token* b) {                                         // added in ch22
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

// resolves a local variable
static int resolveLocal (Compiler* compiler, Token* name) {                                     // added in ch22
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (areIdentifiersEqual(name, &local->name)) {
            if (local->depth == -1)  error("Can't read local variable in its own initializer.");
            return i;
        }
    }

    return -1;
}

// adds an upvalue to the compiler
static int addUpvalue (Compiler* compiler, uint8_t index, bool isLocal) {                      // added in ch25
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) return i; 
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

// resolves an upvalue
static int resolveUpvalue (Compiler* compiler, Token* name) {                                  // added in ch25
    if (compiler->enclosing == NULL) return -1;

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true); 
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) return addUpvalue(compiler, (uint8_t)upvalue, false); 

    return -1;
}

// adds a local variable to the compiler
static void addLocal (Token name) {                                                             // added in ch22
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false; // added in ch25
    // local->depth = current->scopeDepth;
}

// declares a variable in the current scope
static void declareVariable () {                                                                 // added in ch22
    if (current->scopeDepth == 0) return;

    Token* name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) break;
        if (areIdentifiersEqual(name, &local->name)) error("Already variable with this name in this scope."); 
    }

    addLocal(*name);
}

// parses a variable
static uint8_t parseVariable (const char* errorMessage) {                                     // added in ch21
    consumeToken(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();                                                                        // added in ch22
    if (current->scopeDepth > 0) return 0;                                                    // added in ch22

    return identifierConstant(&parser.previous);
}

// marks a variable as initialized
static void markInitialized () { // added in ch22
    if (current->scopeDepth == 0) return; // added in ch24
    current->locals[current->localCount - 1].depth = current->scopeDepth;
} 

// defines global variable
static void defineVariable (uint8_t global) {                                                 // added in ch21
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitTwoBytes(OP_DEFINE_GLOBAL, global);
}

// compiles argument list
static uint8_t argumentList () {                                                              // added in ch24
    uint8_t argCount = 0;
    if (!checkType(TOKEN_CLOSE_PAREN)) {
        do {
            expression();
            if (argCount == 255) error("Can't have more than 255 arguments.");
            argCount++;
        } while (matchType(TOKEN_COMMA));
    }

    consumeToken(TOKEN_CLOSE_PAREN, "Expect ')' after arguments.");
    return argCount;
}

// turns a logical and into a chunk of bytecode
static void and_ (bool canAssign) {                                                           // added in ch23
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);
    patchJump(endJump);
}

// compiles a binary expression
// static void binary() {                                                                      // added in ch17
static void binary (bool canAssign) {                                                          // modified in ch21
    TokenType operatorType = parser.previous.type;
    // Compile the right operand.
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:    emitTwoBytes(OP_EQUAL, OP_NOT);   break; // added in ch18
        case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL);               break; // added in ch18
        case TOKEN_GREATER:       emitByte(OP_GREATER);             break; // added in ch18
        case TOKEN_GREATER_EQUAL: emitTwoBytes(OP_LESS, OP_NOT);    break; // added in ch18
        case TOKEN_LESS:          emitByte(OP_LESS);                break; // added in ch18
        case TOKEN_LESS_EQUAL:    emitTwoBytes(OP_GREATER, OP_NOT); break; // added in ch18
        case TOKEN_PLUS:          emitByte(OP_ADD);                 break;
        case TOKEN_MINUS:         emitByte(OP_SUBTRACT);            break;
        case TOKEN_STAR:          emitByte(OP_MULTIPLY);            break;
        case TOKEN_SLASH:         emitByte(OP_DIVIDE);              break;
        default: return; // Unreachable.
    }
}

// compiles function call
static void call (bool canAssign) {                                                             // added in ch24
    uint8_t argCount = argumentList();
    emitTwoBytes(OP_CALL, argCount);
}

// compiles a dot field access
static void dot (bool canAssign) {                                                              // added in ch25
    consumeToken(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(&parser.previous);

    if (canAssign && matchType(TOKEN_EQUAL)) {
        expression();
        emitTwoBytes(OP_SET_PROPERTY, name);
    } 
    else if (matchType(TOKEN_OPEN_PAREN)) { // added in ch28
        uint8_t argCount = argumentList();
        emitTwoBytes(OP_INVOKE, name);
        emitByte(argCount);
    }
    else { emitTwoBytes(OP_GET_PROPERTY, name); }
}

// compiles a literal expression
// static void literal() {                                                                     // added in ch18
static void literal (bool canAssign) {                                                         // modified in ch21
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL:   emitByte(OP_NIL);   break;
        case TOKEN_TRUE:  emitByte(OP_TRUE);  break;
        default: return; // Unreachable.
    }
}

// compiles a grouping expression
// static void grouping() {                                                                     // added in ch17
static void grouping (bool canAssign) {                                                         // modified in ch21
    expression();
    consumeToken(TOKEN_CLOSE_PAREN, "Expect ')' after expression.");
}

// compiles a number expression
// static void number() {                                                                       // added in ch17
static void number (bool canAssign) {                                                           // modified in ch21 
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));                                                            // added in ch18
    // emitConstant(value);
}

// compiles a logical or into a chunk of bytecode
static void or_ (bool canAssign) {                                                              // added in ch23
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump  = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

// compiles a string expression
// static void string() {
static void string (bool canAssign) {                                                          // modified in ch21
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}
 
// compiles a named var
// static void namedVariable (Token name) {                                                    // added in ch21
static void namedVariable (Token name, bool canAssign) {
    // uint8_t arg = identifierConstant(&name);
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);

    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } 
    else if ((arg = resolveUpvalue(current, &name)) != -1) {                                   // added in ch25
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    }
    else {
        arg   = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    // if (matchType(TOKEN_EQUAL)) {
    if (canAssign && matchType(TOKEN_EQUAL)) {
        expression();
        emitTwoBytes(setOp, (uint8_t)arg);                                                          // added in ch22
        // emitBytes(OP_SET_GLOBAL, arg);
    }
    else /*emitBytes(OP_GET_GLOBAL, arg);*/ emitTwoBytes(getOp, (uint8_t)arg);                      // added in ch22

    // emitBytes(OP_GET_GLOBAL, arg);
}

// compiles a variable expression
// static void variable() { namedVariable(parser.previous); }                                    // added in ch21
static void variable (bool canAssign) {                                                          // added in ch21
    namedVariable(parser.previous, canAssign);
}

// generates a token not found 
static Token syntheticToken (const char* text) {                                                 // added in ch29
    Token token; 
    token.start  = text;
    token.length = (int)strlen(text);
    return token;
}

// compiles a super expression
static void super_ (bool canAssign) {                                                           // added in ch29
    if (currClass == NULL) { error("Can't use 'super' outside of a class."); } 
    else if (!currClass->hasSuperclass) { error("Can't use 'super' in a class with no superclass."); }

    consumeToken(TOKEN_DOT, "Expect '.' after 'super'.");
    consumeToken(TOKEN_IDENTIFIER, "Expect superclass method name.");
    uint8_t name = identifierConstant(&parser.previous);

    namedVariable(syntheticToken("this"), false);
    if (matchType(TOKEN_OPEN_PAREN)) {
        uint8_t argCount = argumentList();
        namedVariable(syntheticToken("super"), false);
        emitTwoBytes(OP_SUPER_INVOKE, name);
        emitByte(argCount);
    } 
    else {
        namedVariable(syntheticToken("super"), false);
        emitTwoBytes(OP_GET_SUPER, name);
    }

    // namedVariable(syntheticToken("super"), false);
    // emitBytes(OP_GET_SUPER, name);
}

// compiles this expression
static void this_ (bool canAssign) {                                                             // added in ch28
    if (currClass == NULL) {
        error("Can't use 'this' outside of a class.");
        return;
    }

    variable(false);
} 

// compiles a unary expression
// static void unary() {                                                                         // added in ch17
static void unary (bool canAssign) {                                                             // modified in ch21
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    // expression();
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_BANG:  emitByte(OP_NOT);    break; // added in ch18
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // Unreachable.
    }
}

// contains the parse rules for each token type
ParseRule rules[] = { // added in ch17
  [TOKEN_OPEN_PAREN]    = {grouping, call,   PREC_CALL},       // updated in ch24
  [TOKEN_CLOSE_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_OPEN_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_CLOSE_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     dot,    PREC_CALL},       // updated in ch25
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},       // updated in ch18
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},   // updated in ch18
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},   // updated in ch18
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON}, // updated in ch18
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON}, // updated in ch18
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON}, // updated in ch18
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON}, // updated in ch18
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},       // updated in ch21
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},       // updated in ch19
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     and_,   PREC_AND},        // updated in ch23
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},       // updated in ch18
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},       // updated in ch18
  [TOKEN_OR]            = {NULL,     or_,    PREC_OR},         // updated in ch23
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {super_,   NULL,   PREC_NONE},       // updated in ch29
  [TOKEN_THIS]          = {this_,    NULL,   PREC_NONE},       // updated in ch28
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},       // updated in ch18
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

// parses precedence for each token type
static void parsePrecedence (Precedence precedence) {                                          // added in ch17
    nextToken();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    // prefixRule();
    bool canAssign = precedence <= PREC_ASSIGNMENT; // added in ch21
    prefixRule(canAssign); // added in ch21

    while (precedence <= getRule(parser.current.type)->precedence) {
        nextToken();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
        // infixRule();
    }

    if (canAssign && matchType(TOKEN_EQUAL)) error("Invalid assignment target."); // added in ch21
}

// grabs the parse rule for a given token type
static ParseRule* getRule (TokenType type) { return &rules[type]; }                             // added in ch17

// parses an expression
static void expression () {                                                                     // added in ch17
  parsePrecedence(PREC_ASSIGNMENT);
}

// parses a block of code
static void block () {                                                                          // added in ch22
    while (!checkType(TOKEN_CLOSE_BRACE) && !checkType(TOKEN_EOF)) { declaration(); }                  

    consumeToken(TOKEN_CLOSE_BRACE, "Expect '}' after block.");
}

// parses a function
static void function (FunctionType type) {                                                      // added in ch24
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consumeToken(TOKEN_OPEN_PAREN, "Expect '(' after function name.");
    if (!checkType(TOKEN_CLOSE_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) errorAtCurrent("Can't have more than 255 parameters.");
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (matchType(TOKEN_COMMA));
    }

    consumeToken(TOKEN_CLOSE_PAREN, "Expect ')' after parameters.");
    consumeToken(TOKEN_OPEN_BRACE, "Expect '{' before function body.");
    block();

    ObjFunction* function = endCompiler();
    emitTwoBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function))); // added in ch25
    // emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        emitTwoBytes(compiler.upvalues[i].isLocal ? 1 : 0, compiler.upvalues[i].index);
    }
}

// parses a method
static void method () {                                                                      // added in ch28
    consumeToken(TOKEN_IDENTIFIER, "Expect method name.");
    uint8_t constant = identifierConstant(&parser.previous);

    // FunctionType type = TYPE_FUNCTION;
    FunctionType type = TYPE_METHOD;

    if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }

    function(type);

    emitTwoBytes(OP_METHOD, constant);
}

// compiles class dec 
static void classDeclaration () {                                                            // added in ch27
    consumeToken(TOKEN_IDENTIFIER, "Expect class name.");
    Token className = parser.previous; // added in ch28
    uint8_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();

    emitTwoBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassCompiler classCompiler;                // added in ch28
    classCompiler.enclosing     = currClass; // added in ch28
    classCompiler.hasSuperclass = false;        // added in ch29
    currClass = &classCompiler;              // added in ch28

    if (matchType(TOKEN_LESS)) {                // added in ch29
        consumeToken(TOKEN_IDENTIFIER, "Expect superclass name.");
        variable(false);
        if (areIdentifiersEqual(&className, &parser.previous)) { error("A class can't inherit from itself."); }

        beginScope();                      // added in ch28
        addLocal(syntheticToken("super")); // added in ch28
        defineVariable(0);                 // added in ch28

        namedVariable(className, false);
        emitByte(OP_INHERIT);
        classCompiler.hasSuperclass = true; 
    }

    namedVariable(className, false); // added in ch28
    consumeToken(TOKEN_OPEN_BRACE, "Expect '{' before class body.");
    while (!checkType(TOKEN_CLOSE_BRACE) && !checkType(TOKEN_EOF)) { method(); } // added in ch28
    consumeToken(TOKEN_CLOSE_BRACE, "Expect '}' after class body.");
    emitByte(OP_POP); // added in ch28

    if (classCompiler.hasSuperclass) { endScope();  } // added in ch29

    currClass = currClass->enclosing; // added in ch28
}

// compiles a function declaration
static void funDeclaration () {                                                                 // added in ch24
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}

// compiles a variable declaration
static void varDeclaration () {                                                                 // added in ch21
  uint8_t global = parseVariable("Expect variable name.");

  if (matchType(TOKEN_EQUAL)) { expression(); }
  else { emitByte(OP_NIL); }

  consumeToken(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  defineVariable(global);
}

// compiles an expression statement
static void expressionStatement () {                                                           // added in ch21
    expression();
    consumeToken(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

// compiles a for loop
static void forStatement () {                                                                  // added in ch23
    beginScope();
    consumeToken(TOKEN_OPEN_PAREN, "Expect '(' after 'for'.");
    // consume(TOKEN_SEMICOLON, "Expect ';'.");
    if (matchType(TOKEN_SEMICOLON)) {} // no init 
    else if (matchType(TOKEN_VAR)) varDeclaration();
    else expressionStatement();

    int loopStart = currChunk()->count;
    // consume(TOKEN_SEMICOLON, "Expect ';'.");
    int exitJump = -1;
    if (!matchType(TOKEN_SEMICOLON)) {
        expression();
        consumeToken(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Condition.
    }    

    // consume(TOKEN_CLOSE_PAREN, "Expect ')' after for clauses.");
    if (!matchType(TOKEN_CLOSE_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currChunk()->count;
        expression();
        emitByte(OP_POP);
        consumeToken(TOKEN_CLOSE_PAREN, "Expect ')' after for clauses.");
        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    statement();
    emitLoop(loopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP); // Condition.
    }

    endScope();
}

// compiles an if statement
static void ifStatement () {                                                                   // added in ch23
    consumeToken(TOKEN_OPEN_PAREN, "Expect '(' after 'if'.");
    expression();
    consumeToken(TOKEN_CLOSE_PAREN, "Expect ')' after condition."); 

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    int elseJump = emitJump(OP_JUMP);
    patchJump(thenJump);
    emitByte(OP_POP);

    if (matchType(TOKEN_ELSE)) { statement(); }
    patchJump(elseJump); 
}

// compiles a print statement
static void printStatement () {                                                                // added in ch21
    expression();
    consumeToken(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

// compiles a return statement
static void returnStatement () {                                                               // added in ch24
    if (current->type == TYPE_SCRIPT) error("Can't return from top-level code."); 

    if (matchType(TOKEN_SEMICOLON))  emitReturn();
    else {
        if (current->type == TYPE_INITIALIZER) { error("Can't return a value from an initializer."); } // added in ch28
        expression();
        consumeToken(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}

// compiles a while loop
static void whileStatement () {                                                                // added in ch23
    int loopStart = currChunk()->count;
    consumeToken(TOKEN_OPEN_PAREN, "Expect '(' after 'while'.");
    expression();
    consumeToken(TOKEN_CLOSE_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);
}

// tries to synch up the parser after an error
static void synchronize () {                                                                    // added in ch21
  parser.panicAtTheDisco = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default: ; // Do nothing.
        }

        nextToken();
    }
}

// compiles a declaration
static void declaration () {                                                                    // added in ch21
//   statement();
    if (matchType(TOKEN_CLASS))    classDeclaration();  // added in ch27
    else if (matchType(TOKEN_FUN)) funDeclaration(); // modified in ch27    
    // if (matchType(TOKEN_FUN))      funDeclaration(); // added in ch24
    else if (matchType(TOKEN_VAR)) varDeclaration(); // modified in ch24
    else statement();

    if (parser.panicAtTheDisco) synchronize();
}

//  compiles a statement
static void statement () {                                                                        // added in ch21
    if      (matchType(TOKEN_PRINT))      printStatement();
    else if (matchType(TOKEN_FOR))        forStatement();                                             // added in ch23
    else if (matchType(TOKEN_IF))         ifStatement();                                              // added in ch23
    else if (matchType(TOKEN_RETURN))     returnStatement();                                          // added in ch24
    else if (matchType(TOKEN_WHILE))      whileStatement();                                           // added in ch23
    else if (matchType(TOKEN_OPEN_BRACE)) {                                                           // added in ch22
        beginScope();
        block();
        endScope();
    }
    else expressionStatement(); 
}

// void compile (const char* source) { 
// bool compile (const char* source, Chunk* chunk) {                                             // updated in ch17

// compiles source code
ObjFunction* compile (const char* source) {                                                     // updated in ch24
    initScanner(source); // only thing that hasn't changed in ch17
    Compiler compiler;         // added in ch22
    initCompiler(&compiler, TYPE_SCRIPT); // added in ch24

    // initCompiler(&compiler);   // added in ch22
    // compilingChunk = chunk; 

    parser.hadError = false;
    parser.panicAtTheDisco = false;

    nextToken();

    while (!matchType(TOKEN_EOF)) { declaration(); } // added in ch21

    ObjFunction* function = endCompiler();    // added in ch24
    return parser.hadError ? NULL : function; // added in ch24

    // expression();
    // consume(TOKEN_EOF, "Expect end of expression.");

    // endCompiler();
    // return !parser.hadError;
    // int line = -1;
    // // for (;;) {
    // while (true) {
    //     Token token = scanToken();
    //     if (token.line != line) {
    //         // printf("%4d ", token.line);
    //         std::cout << std::setw(4) << std::setfill(' ') << token.line << " ";
    //         line = token.line;
    //     } 
    //     else {
    //         // printf("   | ");
    //         std::cout << "   | ";
    //     }
    //     // printf("%2d '%.*s'\n", token.type, token.length, token.start); 
    //     std::cout << std::setw(2) << std::setfill(' ') << token.type << " '" << std::setw(token.length) << std::setfill(' ') << token.start << "'\n";

    //     if (token.type == TOKEN_EOF) break;
}

// marks roots for the garbage collector
void markCompilerRoots () {                                                                             // added in ch25
    Compiler* compiler = current;
    while (compiler != NULL) {
        markObject((Obj*)compiler->function);
        compiler = compiler->enclosing;
    }
}
