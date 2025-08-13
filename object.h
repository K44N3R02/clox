#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "memory.h"
#include "value.h"

enum object_type {
	OBJECT_STRING,
	OBJECT_FUNCTION,
	OBJECT_NATIVE_FN,
	OBJECT_CLOSURE,
	OBJECT_UPVALUE,
};

struct object {
	enum object_type object_type;
	struct object *next;
};

#define OBJECT_TYPE(value) ((value).as.object->object_type)

struct object_string {
	struct object object;
	int32_t length;
	char *characters;
	uint32_t hash;
};

#define IS_STRING(value) (is_object_type(value, OBJECT_STRING))
#define AS_OBJ_STRING(value) ((struct object_string *)AS_OBJECT(value))
#define AS_CSTRING(value) (AS_OBJ_STRING(value)->characters)

struct object_function {
	struct object object;
	int32_t arity;
	int32_t upvalue_count;
	struct chunk chunk;
	struct object_string *name;
};

#define IS_FUNCTION(value) (is_object_type(value, OBJECT_FUNCTION))
#define AS_OBJ_FUNCTION(value) ((struct object_function *)AS_OBJECT(value))

typedef value_t (*native_fn)(int arg_count, value_t *values);

struct object_native_fn {
	struct object object;
	native_fn function;
};

#define IS_NATIVE_FN(value) (is_object_type(value, OBJECT_NATIVE_FN))
#define AS_OBJ_NATIVE_FN(value) \
	(((struct object_native_fn *)AS_OBJECT(value))->function)

struct object_upvalue {
	struct object object;
	value_t *location;
	value_t container;
	struct object_upvalue *next;
};

#define IS_UPVALUE(value) (is_object_type(value, OBJECT_UPVALUE))
#define AS_OBJ_UPVALUE(value) ((struct object_upvalue *)AS_OBJECT(value))

struct object_closure {
	struct object object;
	struct object_function *function;
	struct object_upvalue **upvalues;
	int32_t upvalue_count;
};

#define IS_CLOSURE(value) (is_object_type(value, OBJECT_CLOSURE))
#define AS_OBJ_CLOSURE(value) ((struct object_closure *)AS_OBJECT(value))

bool is_object_type(value_t value, enum object_type object_type);
void print_object(value_t value);

#define FNV_OFFSET_BASIS 2166136261u
#define FNV_PRIME 16777619
int32_t hash(const char *str, int32_t length);

struct object *allocate_object(size_t size, enum object_type obj_type);
struct object_string *copy_string(const char *str, int32_t length);
struct object_string *take_string(char *str, int32_t length);
struct object_function *new_function(void);
struct object_native_fn *new_native_fn(native_fn function);
struct object_upvalue *new_upvalue(value_t *slot);
struct object_closure *new_closure(struct object_function *function);

#endif
