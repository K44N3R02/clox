#include <stddef.h>
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

struct object_string *allocate_string(int32_t length)
{
	struct object_string *result;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
	result = allocate_object(offsetof(struct object_string, characters) +
					 (length + 1) *
						 sizeof(result->characters[0]),
				 OBJECT_STRING);
#pragma clang diagnostic pop
	result->length = length;

	return result;
}

struct object_string *copy_string(const char *str, int32_t length)
{
	struct object_string *result = allocate_string(length);
	memcpy(result->characters, str, length);
	result->characters[length] = '\0';
	return result;
}
