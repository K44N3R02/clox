#include "chunk.h"
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
		struct object_string *str = (struct object_string *)object;
		FREE_ARRAY(char, str->characters, str->length + 1);
		FREE(struct object_string, object);
		break;
	}
	case OBJECT_FUNCTION: {
		struct object_function *fn = (struct object_function *)object;
		free_chunk(&fn->chunk);
		FREE(struct object_function, object);
		// Don't need to free fn->name because of garbage collection
		break;
	}
	case OBJECT_NATIVE_FN:
		FREE(struct object_native_fn, object);
		break;
	case OBJECT_UPVALUE:
		FREE(struct object_upvalue, object);
		break;
	case OBJECT_CLOSURE: {
		struct object_closure *closure =
			(struct object_closure *)object;
		FREE_ARRAY(struct object_upvalue, closure->upvalues,
			   closure->upvalue_count);
		FREE(struct object_closure, object);
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
