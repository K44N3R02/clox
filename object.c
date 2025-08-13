#include "value.h"
#include <string.h>
#include <stdio.h>

#include "chunk.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

bool is_object_type(value_t value, enum object_type object_type)
{
	return IS_OBJECT(value) && AS_OBJECT(value)->object_type == object_type;
}

static void print_function(struct object_function *fn)
{
	if (fn->name == NULL) {
		printf("<script>");
		return;
	}
	printf("<fn %s>", fn->name->characters);
}

void print_object(value_t value)
{
	switch (OBJECT_TYPE(value)) {
	case OBJECT_STRING:
		printf("%s", AS_CSTRING(value));
		break;
	case OBJECT_FUNCTION:
		print_function(AS_OBJ_FUNCTION(value));
		break;
	case OBJECT_NATIVE_FN:
		printf("<native fn>");
		break;
	case OBJECT_CLOSURE:
		print_function(AS_OBJ_CLOSURE(value)->function);
		break;
	case OBJECT_UPVALUE:
		printf("upvalue");
		break;
	default: // UNREACHABLE
		fprintf(stderr, "Unknown object type passed to print_object");
		break;
	}
}

/**
 * FNV-1a
 */
uint32_t hash_string(const char *str, int32_t length)
{
	uint32_t hash = FNV_OFFSET_BASIS, i;

	for (i = 0; i < length; i++) {
		hash ^= (uint8_t)str[i];
		hash *= FNV_PRIME;
	}

	return hash;
}

#define ALLOCATE_OBJ(type, obj_type) \
	((type *)allocate_object(sizeof(type), obj_type))

struct object *allocate_object(size_t size, enum object_type obj_type)
{
	struct object *obj = reallocate(NULL, 0, size);
	obj->object_type = obj_type;
	obj->next = vm.objects;
	vm.objects = obj;
	return obj;
}

static struct object_string *allocate_string(char *str, int32_t length,
					     int32_t hash)
{
	struct object_string *result =
		ALLOCATE_OBJ(struct object_string, OBJECT_STRING);
	result->length = length;
	result->characters = str;
	result->hash = hash;

	table_set(&vm.strings, result, CONS_NIL);

	return result;
}

struct object_string *copy_string(const char *str, int32_t length)
{
	char *copy;
	int32_t hash;
	struct object_string *interned;

	hash = hash_string(str, length);
	interned = table_find_string(&vm.strings, str, length, hash);
	if (interned != NULL)
		return interned;

	copy = ALLOCATE(char, length + 1);
	memcpy(copy, str, length);
	copy[length] = '\0';

	return allocate_string(copy, length, hash);
}

struct object_string *take_string(char *str, int32_t length)
{
	int32_t hash = hash_string(str, length);
	struct object_string *interned =
		table_find_string(&vm.strings, str, length, hash);

	interned = table_find_string(&vm.strings, str, length, hash);
	if (interned != NULL) {
		FREE_ARRAY(char, str, length + 1);
		return interned;
	}

	return allocate_string(str, length, hash);
}

struct object_function *new_function(void)
{
	struct object_function *result =
		ALLOCATE_OBJ(struct object_function, OBJECT_FUNCTION);
	result->arity = 0;
	result->upvalue_count = 0;
	result->name = NULL;
	init_chunk(&result->chunk);
	return result;
}

struct object_native_fn *new_native_fn(native_fn function)
{
	struct object_native_fn *result =
		ALLOCATE_OBJ(struct object_native_fn, OBJECT_NATIVE_FN);
	result->function = function;
	return result;
}

struct object_upvalue *new_upvalue(value_t *slot)
{
	struct object_upvalue *result =
		ALLOCATE_OBJ(struct object_upvalue, OBJECT_UPVALUE);
	result->location = slot;
	result->container = CONS_NIL;
	result->next = NULL;
	return result;
}

struct object_closure *new_closure(struct object_function *function)
{
	struct object_closure *result =
		ALLOCATE_OBJ(struct object_closure, OBJECT_CLOSURE);
	struct object_upvalue **upvalues =
		ALLOCATE(struct object_upvalue *, function->upvalue_count);
	int32_t i;

	for (i = 0; i < function->upvalue_count; i++)
		upvalues[i] = NULL;

	result->function = function;
	result->upvalues = upvalues;
	result->upvalue_count = function->upvalue_count;
	return result;
}
