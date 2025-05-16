#include <string.h>
#include <stdio.h>

#include "object.h"
#include "memory.h"
#include "vm.h"

bool is_object_type(value_t value, enum object_type object_type)
{
	return IS_OBJECT(value) && AS_OBJECT(value)->object_type == object_type;
}

void print_object(value_t value)
{
	switch (OBJECT_TYPE(value)) {
	case OBJECT_STRING:
		printf("%s", AS_CSTRING(value));
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
