#ifndef clox_object_hpp
#define clox_object_hpp

#include "common.hpp"
#include "chunk.hpp" // added in ch24
#include "value.hpp"

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION) // added in ch24
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)   // added in ch24
#define IS_STRING(value)       isObjType(value, OBJ_STRING)

#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))           // added in ch24
#define AS_NATIVE(value)       (((ObjNative*)AS_OBJ(value))->function) // added in ch24
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_FUNCTION, // added in ch24
    OBJ_NATIVE,   // added in ch24
    OBJ_STRING,
} ObjType;

struct Obj {    
    struct Obj* next;
    ObjType     type;
};

typedef struct { // added in ch24
    Obj        obj;
    int        arity;
    Chunk      chunk;
    ObjString* name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args); // added in ch24
 
typedef struct { // added in ch24
    Obj      obj; 
    NativeFn function;
} ObjNative;

struct ObjString {
    Obj      obj;
    int      length;
    char*    chars;
    uint32_t hash; // added in ch20
};

ObjFunction*       newFunction ();                  // added in ch24
ObjNative*         newNative   (NativeFn function); // added in ch24
ObjString*         takeString  (char* chars, int length);
ObjString*         copyString  (const char* chars, int length);
void               printObject (Value value);
static inline bool isObjType   (Value value, ObjType type) { return IS_OBJ(value) && AS_OBJ(value)->type == type; }

#endif