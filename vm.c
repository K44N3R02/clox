#include "chunk.h"
#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
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
}

void free_vm(void)
{
}

static enum interpret_result run(void)
{
#define READ_BYTE() (*vm.ip++)
#define FETCH_CONST(address) (vm.chunk->constants.values[(address)])
#define BINARY_OP(op)         \
	do {                  \
		b = pop();    \
		a = pop();    \
		push(a op b); \
	} while (0)

	uint8_t instruction;
	int32_t address;
	value_t constant, a, b;
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
			address = (READ_BYTE() << 16) + (READ_BYTE() << 8) +
				  READ_BYTE();
			constant = FETCH_CONST(address);
			push(constant);
			break;
		case OP_NEGATE:
			vm.stack_top[-1] *= -1;
			break;
		case OP_ADD:
			BINARY_OP(+);
			break;
		case OP_SUB:
			BINARY_OP(-);
			break;
		case OP_MUL:
			BINARY_OP(*);
			break;
		case OP_DIV:
			BINARY_OP(/);
			break;
		case OP_RETURN:
			print_value(pop());
			printf("\n");
			return INTERPRET_OK;
		}
	}
#undef READ_BYTE
#undef FETCH_CONST
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
