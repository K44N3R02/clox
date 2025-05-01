#include "common.h"
#include "memory.h"

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
