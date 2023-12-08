#ifndef clox_object_hpp
#define clox_object_hpp

#include "common.hpp"
#include "chunk.hpp" // added in ch24
#include "table.hpp" // added in ch27
#include "value.hpp"

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)    // added in ch27
#define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)  // added in ch25
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION) // added in ch24
#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE) // added in ch27
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)   // added in ch24
#define IS_STRING(value)       isObjType(value, OBJ_STRING)

#define AS_CLASS(value)        ((ObjClass*)AS_OBJ(value))              // added in ch27
#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))            // added in ch25
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))           // added in ch24
#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))           // added in ch27
#define AS_NATIVE(value)       (((ObjNative*)AS_OBJ(value))->function) // added in ch24
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_CLASS,    // added in ch27
    OBJ_CLOSURE,  // added in ch25
    OBJ_FUNCTION, // added in ch24
    OBJ_INSTANCE, // added in ch27
    OBJ_NATIVE,   // added in ch24
    OBJ_STRING,
    OBJ_UPVALUE   // added in ch25
} ObjType;

struct Obj {    
    struct Obj* next;
    bool        isMarked; // added in ch26
    ObjType     type;
};

typedef struct { // added in ch24
    Obj        obj;
    int        arity;
    int        upvalueCount; // added in ch25
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

typedef struct ObjUpvalue { // added in ch25
    Obj                obj;
    Value*             location;
    Value              closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct { // added in ch25
    Obj          obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int          upvalueCount;
} ObjClosure;

typedef struct { // added in ch27
    Obj obj;
    ObjString* name;
} ObjClass;

typedef struct { // added in ch27
    Obj       obj;
    ObjClass* klass;
    Table     fields; 
} ObjInstance;

ObjClass*          newClass    (ObjString* name);       // added in ch27
ObjClosure*        newClosure  (ObjFunction* function); // added in ch25
ObjFunction*       newFunction ();                      // added in ch24
ObjInstance*       newInstance (ObjClass* klass);       // added in ch27
ObjNative*         newNative   (NativeFn function);     // added in ch24
ObjString*         takeString  (char* chars, int length);
ObjString*         copyString  (const char* chars, int length);
ObjUpvalue*        newUpvalue  (Value* slot);           // added in ch25
void               printObject (Value value);
static inline bool isObjType   (Value value, ObjType type) { return IS_OBJ(value) && AS_OBJ(value)->type == type; }

#endif