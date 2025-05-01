#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef double value_t;

struct value_array {
	int32_t length;
	int32_t capacity;
	value_t *values;
};

void init_value_array(struct value_array *value_array);
void write_value_array(struct value_array *value_array, value_t value);
void free_value_array(struct value_array *value_array);

void print_value(value_t value);

#endif
