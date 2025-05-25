#include "memory.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "chunk.h"
#include "debug.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

struct vm vm;

static void reset_stack(void)
{
	vm.stack_top = vm.stack;
}

void init_vm(void)
{
	reset_stack();
	vm.objects = NULL;
	init_table(&vm.globals);
	init_table(&vm.strings);
}

void free_vm(void)
{
	free_objects();
	free_table(&vm.strings);
}

static value_t peek(int32_t distance)
{
	return vm.stack_top[-1 - distance];
}

static void runtime_error(const char *format, ...)
{
	int32_t instruction;
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");

	instruction = vm.ip - vm.chunk->code - 1;
	fprintf(stderr, "[line: %d] in script\n",
		read_line(&vm.chunk->lines, instruction));
	reset_stack();
}

static bool is_false(value_t value)
{
	return IS_NIL(value) || (IS_BOOLEAN(value) && !AS_BOOLEAN(value));
}

static bool is_equal(value_t a, value_t b)
{
	if (a.value_type != b.value_type)
		return false;

	switch (a.value_type) {
	case VALUE_NUMBER:
		return AS_NUMBER(a) == AS_NUMBER(b);
	case VALUE_BOOLEAN:
		return AS_BOOLEAN(a) == AS_BOOLEAN(b);
	case VALUE_NIL:
		return true;
	case VALUE_OBJECT:
		return AS_OBJECT(a) == AS_OBJECT(b);
	default:
		return false;
	}
}

static void concatenate(void)
{
	struct object_string *a, *b, *result;
	int32_t length;
	char *buffer;

	b = AS_OBJ_STRING(pop());
	a = AS_OBJ_STRING(pop());
	length = a->length + b->length;
	buffer = ALLOCATE(char, length + 1);
	memcpy(buffer, a->characters, a->length);
	memcpy(buffer + a->length, b->characters, b->length);
	buffer[length] = '\0';

	result = take_string(buffer, length);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
	push(CONS_OBJECT(result));
#pragma clang diagnostic pop
}

static enum interpret_result run(void)
{
#define READ_BYTE() (*vm.ip++)
#define READ_LONG_ARG() ((READ_BYTE() << 16) + (READ_BYTE() << 8) + READ_BYTE())
#define FETCH_CONST(address) (vm.chunk->constants.values[(address)])
#define READ_STRING(address) (AS_OBJ_STRING(FETCH_CONST((address))))
#define BINARY_OP(result_type, op)                                            \
	do {                                                                  \
		if (!(IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))) {            \
			runtime_error("Binary %s requires two numbers", #op); \
			return INTERPRET_RUNTIME_ERROR;                       \
		}                                                             \
		b = pop();                                                    \
		a = pop();                                                    \
		push(result_type(AS_NUMBER(a) op AS_NUMBER(b)));              \
	} while (0)

	uint8_t instruction;
	int32_t address;
	value_t constant, a, b;
	struct object_string *name;
#ifdef DEBUG_TRACE_EXECUTION
	value_t *slot;
#endif

	for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
		printf("        ");
		for (slot = vm.stack; slot < vm.stack_top; slot++) {
			printf("[ ");
			print_value(*slot);
			printf(" ]");
		}
		printf("\n");
		disassemble_instruction(vm.chunk,
					(int32_t)(vm.ip - vm.chunk->code));
#endif
		switch (instruction = READ_BYTE()) {
		case OP_CONSTANT:
			address = READ_BYTE();
			constant = FETCH_CONST(address);
			push(constant);
			break;
		case OP_CONSTANT_LONG:
			address = READ_LONG_ARG();
			constant = FETCH_CONST(address);
			push(constant);
			break;
		case OP_NIL:
			push(CONS_NIL);
			break;
		case OP_TRUE:
			push(CONS_BOOLEAN(true));
			break;
		case OP_FALSE:
			push(CONS_BOOLEAN(false));
			break;
		case OP_NOT:
			push(CONS_BOOLEAN(is_false(pop())));
			break;
		case OP_NEGATE:
			if (!IS_NUMBER(peek(0))) {
				runtime_error(
					"Unary negation requires a number.");
				return INTERPRET_RUNTIME_ERROR;
			}

			AS_NUMBER(vm.stack_top[-1]) *= -1;
			break;
		case OP_ADD:
			if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
				b = pop();
				a = pop();
				push(CONS_NUMBER(AS_NUMBER(a) + AS_NUMBER(b)));
			} else if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
				concatenate();
			} else {
				runtime_error(
					"Binary + requires two numbers or two strings");
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		case OP_SUB:
			BINARY_OP(CONS_NUMBER, -);
			break;
		case OP_MUL:
			BINARY_OP(CONS_NUMBER, *);
			break;
		case OP_DIV:
			BINARY_OP(CONS_NUMBER, /);
			break;
		case OP_EQUAL:
			b = pop();
			a = pop();
			push(CONS_BOOLEAN(is_equal(a, b)));
			break;
		case OP_LESS:
			BINARY_OP(CONS_BOOLEAN, <);
			break;
		case OP_GREATER:
			BINARY_OP(CONS_BOOLEAN, >);
			break;
		case OP_PRINT:
			print_value(pop());
			printf("\n");
			break;
		case OP_POP:
			pop();
			break;
		case OP_DEFINE_GLOBAL:
			name = READ_STRING(READ_BYTE());
			table_set(&vm.globals, name, peek(0));
			// Garbage collection may trigger, pop after writing
			// to table is completed
			pop();
			break;
		case OP_DEFINE_GLOBAL_LONG:
			name = READ_STRING(READ_LONG_ARG());
			table_set(&vm.globals, name, peek(0));
			// Garbage collection may trigger, pop after writing
			// to table is completed
			pop();
			break;
		case OP_GET_GLOBAL:
			name = READ_STRING(READ_BYTE());
			if (!table_get(&vm.globals, name, &a)) {
				runtime_error("Undefined variable '%s'.",
					      name->characters);
				return INTERPRET_RUNTIME_ERROR;
			}
			push(a);
			break;
		case OP_GET_GLOBAL_LONG:
			name = READ_STRING(READ_LONG_ARG());
			if (!table_get(&vm.globals, name, &a)) {
				runtime_error("Undefined variable '%s'.",
					      name->characters);
				return INTERPRET_RUNTIME_ERROR;
			}
			push(a);
			break;
		case OP_SET_GLOBAL:
			name = READ_STRING(READ_BYTE());
			// This disables implicit variable declaration
			if (table_set(&vm.globals, name, peek(0))) {
				table_delete(&vm.globals, name);
				runtime_error("Undefined variable '%s'.",
					      name->characters);
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		case OP_RETURN:
			return INTERPRET_OK;
		}
	}
#undef READ_BYTE
#undef READ_LONG_ARG
#undef FETCH_CONST
#undef READ_STRING
#undef BINARY_OP
}

// TODO: Go through the chunk and resize the stack size of vm accordingly
enum interpret_result interpret(char *source)
{
	struct chunk chunk;
	enum interpret_result result;

	init_chunk(&chunk);

	if (!compile(source, &chunk)) {
		result = INTERPRET_COMPILE_ERROR;
		goto out_free_chunk;
	}

	vm.chunk = &chunk;
	vm.ip = vm.chunk->code;

	result = run();

out_free_chunk:
	free_chunk(&chunk);
	return result;
}

void push(value_t value)
{
	*vm.stack_top = value;
	vm.stack_top++;
}

value_t pop(void)
{
	vm.stack_top--;
	return *vm.stack_top;
}
