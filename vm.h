#ifndef clox_vm_h
#define clox_vm_h

#include "common.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define FRAME_MAX 64
#define STACK_MAX (FRAME_MAX * (1 << 8))

enum interpret_result {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
};

struct call_frame {
	struct object_function *function;
	uint8_t *ip;
	value_t *slots;
};

struct vm {
	struct call_frame frames[FRAME_MAX];
	int32_t frame_count;
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
