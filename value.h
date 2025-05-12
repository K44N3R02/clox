#ifndef clox_value_h
#define clox_value_h

#include "common.h"

enum value_type {
	VALUE_NUMBER,
	VALUE_BOOLEAN,
	VALUE_NIL,
};

struct value {
	enum value_type value_type;
	union {
		bool boolean;
		double number;
	} as;
};

#define CONS_NUMBER(val) ((struct value){ VALUE_NUMBER, { .number = val } })
#define CONS_BOOLEAN(val) ((struct value){ VALUE_BOOLEAN, { .boolean = val } })
#define CONS_NIL ((struct value){ VALUE_NIL, { .number = 0 } })

#define AS_NUMBER(value) ((value).as.number)
#define AS_BOOLEAN(value) ((value).as.boolean)

#define IS_NUMBER(value) ((value).value_type == VALUE_NUMBER)
#define IS_BOOLEAN(value) ((value).value_type == VALUE_BOOLEAN)
#define IS_NIL(value) ((value).value_type == VALUE_NIL)

typedef struct value value_t;

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
