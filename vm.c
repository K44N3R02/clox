#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "chunk.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

struct vm vm;

static void define_native_fn(const char *name, native_fn function)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
	push(CONS_OBJECT(copy_string(name, (int32_t)strlen(name))));
	push(CONS_OBJECT(new_native_fn(function)));
#pragma clang diagnostic pop
	table_set(&vm.globals, AS_OBJ_STRING(vm.stack[0]), vm.stack[1]);
	pop();
	pop();
}

static value_t clock_native(int arg_count, value_t *args)
{
	return CONS_NUMBER((double)clock() / CLOCKS_PER_SEC);
}

static void reset_stack(void)
{
	vm.stack_top = vm.stack;
	vm.frame_count = 0;
}

void init_vm(void)
{
	reset_stack();
	vm.objects = NULL;
	init_table(&vm.globals);
	init_table(&vm.strings);
	define_native_fn("clock", clock_native);
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
	int32_t instruction, i;
	struct call_frame *frame;
	struct object_function *function;
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");

	for (i = vm.frame_count - 1; i >= 0; i--) {
		frame = &vm.frames[i];
		function = frame->function;
		instruction = frame->ip - function->chunk.code - 1;
		fprintf(stderr, "[line %d] in ",
			read_line(&function->chunk.lines, instruction));
		if (function->name == NULL)
			fprintf(stderr, "script\n");
		else
			fprintf(stderr, "%s()\n", function->name->characters);
	}
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

static bool call(struct object_function *function, int32_t arg_count)
{
	struct call_frame *frame;

	if (function->arity != arg_count) {
		runtime_error("Expected %d arguments, got %d.", function->arity,
			      arg_count);
		return false;
	}
	if (vm.frame_count == FRAME_MAX) {
		runtime_error("Stack overflow.");
		return false;
	}
	frame = &vm.frames[vm.frame_count++];
	frame->function = function;
	frame->ip = function->chunk.code;
	frame->slots = vm.stack_top - arg_count - 1;
	return true;
}

static bool call_value(value_t value, int32_t arg_count)
{
	if (IS_OBJECT(value)) {
		switch (OBJECT_TYPE(value)) {
		case OBJECT_FUNCTION:
			return call(AS_OBJ_FUNCTION(value), arg_count);
		case OBJECT_NATIVE_FN: {
			native_fn native = AS_OBJ_NATIVE_FN(value);
			value_t result =
				native(arg_count, vm.stack_top - arg_count);
			vm.stack_top -= arg_count + 1;
			push(result);
			return true;
		}
		default:
			break;
		}
	}
	runtime_error("Only functions and classes can be called.");
	return false;
}

static enum interpret_result run(void)
{
	struct call_frame *frame = &vm.frames[vm.frame_count - 1];
#define READ_BYTE() (*frame->ip++)
#define READ_UINT16() \
	(frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_LONG_ARG() ((READ_BYTE() << 16) + (READ_BYTE() << 8) + READ_BYTE())
#define FETCH_CONST(address) \
	(frame->function->chunk.constants.values[(address)])
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

	uint8_t instruction, local;
	int32_t address, arg_count;
	value_t constant, a, b, result;
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
		disassemble_instruction(
			&frame->function->chunk,
			(int32_t)(frame->ip - frame->function->chunk.code));
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
		case OP_POPN:
			vm.stack_top -= READ_BYTE();
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
		case OP_GET_LOCAL:
			local = READ_BYTE();
			push(frame->slots[local]);
			break;
		case OP_SET_LOCAL:
			local = READ_BYTE();
			frame->slots[local] = peek(0);
			break;
		case OP_JUMP_IF_FALSE:
			address = READ_UINT16();
			if (is_false(peek(0)))
				frame->ip += address;
			// frame->ip += is_false(peek(0)) * address;
			break;
		case OP_JUMP:
			address = READ_UINT16();
			frame->ip += address;
			break;
		case OP_LOOP:
			address = READ_UINT16();
			frame->ip -= address;
			break;
		case OP_CALL:
			arg_count = READ_BYTE();
			if (!call_value(peek(arg_count), arg_count))
				return INTERPRET_RUNTIME_ERROR;
			frame = &vm.frames[vm.frame_count - 1];
			break;
		case OP_RETURN:
			result = pop();
			vm.frame_count--;
			if (vm.frame_count == 0) {
				pop();
				return INTERPRET_OK;
			}
			vm.stack_top = frame->slots;
			push(result);
			frame = &vm.frames[vm.frame_count - 1];
			break;
		}
	}
#undef READ_BYTE
#undef READ_UINT16
#undef READ_LONG_ARG
#undef FETCH_CONST
#undef READ_STRING
#undef BINARY_OP
}

// TODO: Go through the chunk and resize the stack size of vm accordingly
enum interpret_result interpret(char *source)
{
	struct object_function *function;

	function = compile(source);
	if (function == NULL)
		return INTERPRET_COMPILE_ERROR;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
	push(CONS_OBJECT(function));
#pragma clang diagnostic pop
	call(function, 0);

	return run();
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
