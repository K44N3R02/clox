#include <stdio.h>

#include "value.h"
#include "memory.h"

/**
 * NOTE: This function does no allocation.
 */
void init_value_array(struct value_array *value_array)
{
	value_array->length = 0;
	value_array->capacity = 0;
	value_array->values = NULL;
}

/**
 * NOTE: The @value_array must be properly initialized before calling this
 * function.
 */
void write_value_array(struct value_array *value_array, value_t value)
{
	int32_t old_capacity;

	if (value_array->capacity < value_array->length + 1) {
		old_capacity = value_array->capacity;
		value_array->capacity = GROW_CAPACITY(old_capacity);
		value_array->values = GROW_ARRAY(value_t, value_array->values,
						 old_capacity,
						 value_array->capacity);
	}

	value_array->values[value_array->length] = value;
	value_array->length++;
}

void free_value_array(struct value_array *value_array)
{
	FREE_ARRAY(value_t, value_array->values, value_array->capacity);
	init_value_array(value_array);
}

void print_value(value_t value)
{
	switch (value.value_type) {
	case VALUE_NUMBER:
		printf("%g", AS_NUMBER(value));
		break;
	case VALUE_BOOLEAN:
		printf(AS_BOOLEAN(value) ? "true" : "false");
		break;
	case VALUE_NIL:
		printf("nil");
		break;
	}
}
