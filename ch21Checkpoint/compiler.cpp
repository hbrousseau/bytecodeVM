#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <stdlib.h> // added in ch17

#include "common.hpp"
#include "compiler.hpp"
#include "object.hpp" // added in ch19
#include "scanner.hpp"

#ifdef DEBUG_PRINT_CODE // added in ch17
#include "debug.hpp"
#endif

typedef struct { // added in ch17
    Token current;
    Token previous;
    bool hadError;
    bool panicAtTheDisco;
} Parser;

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
typedef void (*ParseFn)(bool canAssign); // updated in ch21

typedef struct { // added in ch17... Pratt parser
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

Parser parser; // added in ch17
Chunk* compilingChunk; // added in ch17

static Chunk* currChunk() { return compilingChunk; } // added in ch17

static void errorAt(Token* token, const char* message) { // added in ch17
    if (parser.panicAtTheDisco) return; // stop cascading errors
    parser.panicAtTheDisco = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) fprintf(stderr, " at end");
    else if (token->type == TOKEN_ERROR) { /*Nothing*/ }
    else fprintf(stderr, " at '%.*s'", token->length, token->start);

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char* message) { errorAt(&parser.previous, message); } // added in ch17
static void errorAtCurrent(const char* message) { errorAt(&parser.current, message); } // added in ch17

static void nextToken() { // added in ch17
    parser.previous = parser.current;

    // for (;;) {
    while (true) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static void consumeToken(TokenType type, const char* message) { // added in ch17
    if (parser.current.type == type) {
        nextToken();
        return;
    }

    errorAtCurrent(message);
}

static bool checkType(TokenType type) { return parser.current.type == type; } // added in ch21

static bool matchType (TokenType type) { // added in ch21   
    if (!checkType(type)) return false;
    nextToken();
    return true;
}

static void emitByte(uint8_t byte) { writeChunk(currChunk(), byte, parser.previous.line); } // added in ch17
static void emitTwoBytes(uint8_t byte1, uint8_t byte2) { emitByte(byte1); emitByte(byte2); }      // added in ch17
static void emitReturn() { emitByte(OP_RETURN); }                                              // added in ch17

static uint8_t makeConstant(Value value) {                                                     // added in ch17
    int constant = addConstant(currChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value) { emitTwoBytes(OP_CONSTANT, makeConstant(value)); }         // added in ch17

static void endCompiler() {                                                                    // added in ch17 
    emitReturn(); 
    #ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) disassembleChunk(currChunk(), "code");
    #endif
}

static void expression();                                                                      // added in ch17
static void statement();                                                                       // added in ch21
static void declaration();                                                                     // added in ch21
static ParseRule* getRule(TokenType type);                                                     // added in ch17
static void parsePrecedence(Precedence precedence);                                            // added in ch17

// static void binary() {                                                                      // added in ch17
static void binary (bool canAssign) {                                                          // modified in ch21
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:    emitTwoBytes(OP_EQUAL, OP_NOT);   break; // added in ch18
        case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL);            break; // added in ch18
        case TOKEN_GREATER:       emitByte(OP_GREATER);          break; // added in ch18
        case TOKEN_GREATER_EQUAL: emitTwoBytes(OP_LESS, OP_NOT);    break; // added in ch18
        case TOKEN_LESS:          emitByte(OP_LESS);             break; // added in ch18
        case TOKEN_LESS_EQUAL:    emitTwoBytes(OP_GREATER, OP_NOT); break; // added in ch18
        case TOKEN_PLUS:          emitByte(OP_ADD);              break;
        case TOKEN_MINUS:         emitByte(OP_SUBTRACT);         break;
        case TOKEN_STAR:          emitByte(OP_MULTIPLY);         break;
        case TOKEN_SLASH:         emitByte(OP_DIVIDE);           break;
        default: return; // Unreachable.
    }
}

// static void literal() {                                                                     // added in ch18
static void literal (bool canAssign) {                                                         // modified in ch21
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        default: return; // Unreachable.
    }
}

// static void grouping() {                                                                     // added in ch17
static void grouping (bool canAssign) {                                                         // modified in ch21
    expression();
    consumeToken(TOKEN_CLOSE_PAREN, "Expect ')' after expression.");
}

// static void number() {                                                                       // added in ch17
static void number (bool canAssign) {                                                           // modified in ch21 
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));                                                              // added in ch18
  // emitConstant(value);
}

// static void string() {
static void string(bool canAssign) {                                                           // modified in ch21
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static uint8_t identifierConstant(Token* name) { return makeConstant(OBJ_VAL(copyString(name->start, name->length))); } // added in ch21
 
// static void namedVariable (Token name) {                                                       // added in ch21
static void namedVariable (Token name, bool canAssign) {
    uint8_t arg = identifierConstant(&name);

    // if (matchType(TOKEN_EQUAL)) {
    if (canAssign && matchType(TOKEN_EQUAL)) {
        expression();
        emitTwoBytes(OP_SET_GLOBAL, arg);
    }
    else emitTwoBytes(OP_GET_GLOBAL, arg);

    // emitTwoBytes(OP_GET_GLOBAL, arg);
}

// static void variable() { namedVariable(parser.previous); }                                    // added in ch21
static void variable(bool canAssign) {                                                           // added in ch21
    namedVariable(parser.previous, canAssign);
}

// static void unary() {                                                                         // added in ch17
static void unary (bool canAssign) {                                                             // modified in ch21
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    // expression();
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_BANG:  emitByte(OP_NOT); break; // added in ch18
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // Unreachable.
    }
}

ParseRule rules[] = { // added in ch17
  [TOKEN_OPEN_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_CLOSE_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_OPEN_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_CLOSE_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
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
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},       // updated in ch18
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},       // updated in ch18
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},       // updated in ch18
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

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

static uint8_t parseVariable(const char* errorMessage) {                                       // added in ch21
    consumeToken(TOKEN_IDENTIFIER, errorMessage);
    return identifierConstant(&parser.previous);
}

static void defineVariable(uint8_t global) { emitTwoBytes(OP_DEFINE_GLOBAL, global); }            // added in ch21

static ParseRule* getRule (TokenType type) { return &rules[type]; }                            // added in ch17

static void expression() {                                                                     // added in ch17
  parsePrecedence(PREC_ASSIGNMENT);
}

static void varDeclaration() {                                                                 // added in ch21
  uint8_t global = parseVariable("Expect variable name.");

  if (matchType(TOKEN_EQUAL)) expression();
  else emitByte(OP_NIL);
  consumeToken(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  defineVariable(global);
}

static void expressionStatement() {                                                            // added in ch21
    expression();
    consumeToken(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void printStatement() {                                                                  // added in ch21
    expression(); 
    consumeToken(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}
 
static void synchronize() {                                                                     // added in ch21
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

            default:
                ; // Do nothing.
        }

        nextToken();
    }
}

static void declaration() {                                                                     // added in ch21
//   statement();
    if (matchType(TOKEN_VAR)) varDeclaration();
    else statement();

  if (parser.panicAtTheDisco) synchronize();
}

static void statement() {                                                                       // added in ch21
  if (matchType(TOKEN_PRINT)) printStatement(); 
  else expressionStatement(); 
}

// void compile (const char* source) { 
bool compile (const char* source, Chunk* chunk) {                                               // updated in ch17
    initScanner(source); // only thing that hasn't changed in ch17
    compilingChunk = chunk; 

    parser.hadError = false;
    parser.panicAtTheDisco = false;

    nextToken();

    while (!matchType(TOKEN_EOF)) {                                                                // updated in ch21 
        declaration();
    }

    // expression();
    // consumeToken(TOKEN_EOF, "Expect end of expression.");

    endCompiler();
    return !parser.hadError;
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