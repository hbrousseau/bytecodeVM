#include <stdio.h>
#include <string.h>

#include "common.hpp"
#include "scanner.hpp"

// represents scanner
typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner; // global scanner variable

// initializes scanner
void initScanner (const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAlpha (char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; } // if c is a letter or underscore

static bool isDigit (char c) { return c >= '0' && c <= '9'; } // if c is a digit

static bool isAtEnd () { return *scanner.current == '\0'; } // if we've reached the end of the file

// grabs the next token
static char nextToken () {
    scanner.current++;
    return scanner.current[-1];
}

static char peek () { return *scanner.current; } // peeks at the current character

// peeks at the next character
static char peekNext () { 
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

// checks if the current character matches the expected character
static bool matchMe (char expected) { 
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

// creates a token
static Token makeToken (TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

// creates an error token
static Token errorToken (const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

// skips whitespace
static void skipWhitespace () {
    // for (;;) {
    while (true) {
        char curr = peek();
        switch (curr) {
            case ' ':
            case '\r':
            case '\t':
                nextToken();
                break;

            case '\n': // keep track of line numbers
                scanner.line++; 
                nextToken();
                break;

            case '/':
                if (peekNext() == '/') {
                    // A comment goes until the end of the line.
                    while (peek() != '\n' && !isAtEnd()) nextToken();
                } 
                else return;
                break;

            default:
                return;
        }
    }
}

// checks if the current character is a keyword
static TokenType checkKeyword (int start, int length,
    const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0) return type;

    return TOKEN_IDENTIFIER;
}

// checks if the current character is an identifier
static TokenType identifierType () {
    switch (scanner.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }
    
    return TOKEN_IDENTIFIER;
}

// checks if the current token is an identifier
static Token identifier () {
    while (isAlpha(peek()) || isDigit(peek())) nextToken();
    return makeToken(identifierType());
}

// if the current token is a digit, it's a number token 
static Token number () {
    while (isDigit(peek())) nextToken();

    // Look for a fractional part
    if (peek() == '.' && isDigit(peekNext())) { // Consume the "."
        nextToken();
        while (isDigit(peek())) nextToken();
    }

    return makeToken(TOKEN_NUMBER);
}

// if the current token is a string, it's a string token
static Token string () {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        nextToken();
    }

    if (isAtEnd()) return errorToken("Unterminated string.");

    // The closing quote.
    nextToken();
    return makeToken(TOKEN_STRING);
}

// scannner function
Token scanToken () {
    skipWhitespace();
    scanner.start = scanner.current;
    if (isAtEnd()) return makeToken(TOKEN_EOF);

    char curr = nextToken();
    if (isAlpha(curr)) return identifier();
    if (isDigit(curr)) return number();
    switch (curr) {
        case '(': return makeToken(TOKEN_OPEN_PAREN);
        case ')': return makeToken(TOKEN_CLOSE_PAREN);
        case '{': return makeToken(TOKEN_OPEN_BRACE);
        case '}': return makeToken(TOKEN_CLOSE_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);

        case '!':
            return makeToken(matchMe('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(matchMe('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(matchMe('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(matchMe('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

         case '"': return string();
    }

    return errorToken("Unexpected character.");
}