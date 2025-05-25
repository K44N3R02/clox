#ifndef clox_vm_h
#define clox_vm_h

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

#define STACK_MAX 256

enum interpret_result {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
};

struct vm {
	struct chunk *chunk;
	uint8_t *ip;
	value_t stack[STACK_MAX];
	value_t *stack_top;
	struct table strings;
	struct table globals;
	struct object *objects;
};

extern struct vm vm;

void init_vm(void);
void free_vm(void);
enum interpret_result interpret(char *source);

void push(value_t value);
value_t pop(void);

#endif
