#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "memory.h"
#include "value.h"

enum object_type {
	OBJECT_STRING,
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

bool is_object_type(value_t value, enum object_type object_type);
void print_object(value_t value);

#define FNV_OFFSET_BASIS 2166136261u
#define FNV_PRIME 16777619
int32_t hash(const char *str, int32_t length);

struct object *allocate_object(size_t size, enum object_type obj_type);
struct object_string *copy_string(const char *str, int32_t length);
struct object_string *take_string(char *str, int32_t length);

#endif
