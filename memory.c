#include "common.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

/**
 * reallocate() - Resizes a memory block.
 * @ptr: Pointer to the memory block to be resized.
 * @old_size: The current size of the memory block (not used in this function).
 * @new_size: The new size for the memory block.
 *
 * This function attempts to resize the memory block pointed to by @ptr
 * to the specified @new_size. If @new_size is zero, the memory block
 * is freed and NULL is returned.
 *
 * NOTE: If the reallocation fails, the program exits with a status of 1.
 *
 * NOTE: The caller is responsible for ensuring that @ptr is a valid
 * pointer returned by a previous allocation function (e.g., malloc or
 * realloc).
 *
 * NOTE: The @old_size parameter is included for compatibility but
 * is not used in the function's logic.
 *
 * Return: A pointer to the newly allocated memory block, or NULL if
 * the new size is zero.
 */
void *reallocate(void *ptr, size_t old_size, size_t new_size)
{
	void *result;

	if (new_size == 0) {
		free(ptr);
		return NULL;
	}

	result = realloc(ptr, new_size);
	if (result == NULL)
		exit(1);

	return result;
}

static void free_object(struct object *object)
{
	switch (object->object_type) {
	case OBJECT_STRING: {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
		struct object_string *str = object;
#pragma clang diagnostic pop
		FREE_ARRAY(char, str->characters, str->length + 1);
		FREE(struct object_string, object);
		break;
	}
	default:
		break;
	}
}

void free_objects(void)
{
	struct object *object = vm.objects;
	while (object != NULL) {
		vm.objects = object->next;
		free_object(object);
		object = vm.objects;
	}
}
