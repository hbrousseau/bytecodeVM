#ifndef clox_common_hpp
#define clox_common_hpp

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// nan-boxing is a technique for storing values in a single 64-bit word
#define NAN_BOXING                  // added in ch30

// #define DEBUG_PRINT_CODE            // added in ch17

// #define DEBUG_TRACE_EXECUTION       // added in ch15

// #define DEBUG_STRESS_GC             // added in ch26
// #define DEBUG_LOG_GC                // added in ch26

// this is a macro that will be used to print the line number and file name of the code that caused the error
#define UINT8_COUNT (UINT8_MAX + 1) // added in ch22

#endif