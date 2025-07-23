#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "object.h"
#include "value.h"

static void print_type(value_t value)
{
	switch (value.value_type) {
	case VALUE_NUMBER:
		printf("number");
		break;
	case VALUE_BOOLEAN:
		printf("boolean");
		break;
	case VALUE_NIL:
		printf("nil");
		break;
	case VALUE_OBJECT:
		printf("object");
		switch (AS_OBJECT(value)->object_type) {
		case OBJECT_STRING:
			printf(">string");
			break;
		default:
			fprintf(stderr,
				"Unknown object type given to print_type.");
			break;
		}
		break;
	default:
		fprintf(stderr, "Unknown value type given to print_type.");
		break;
	}
}

static void repr_value(value_t value)
{
	printf("'");
	print_value(value);
	printf("'");
	switch (value.value_type) {
	case VALUE_NUMBER:
		break;
	case VALUE_BOOLEAN:
		break;
	case VALUE_NIL:
		break;
	case VALUE_OBJECT:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-pedantic"
		printf(" obj:%p next:%p", AS_OBJECT(value),
		       AS_OBJECT(value)->next);
		switch (AS_OBJECT(value)->object_type) {
		case OBJECT_STRING:
			printf(" len:%4d chr-ptr:%p hash:%08x",
			       AS_OBJ_STRING(value)->length,
			       AS_OBJ_STRING(value)->characters,
			       AS_OBJ_STRING(value)->hash);
			break;
		default:
			fprintf(stderr,
				"Unknown object type given to repr_value.");
			break;
		}
		break;
#pragma clang diagnostic pop
	default:
		fprintf(stderr, "Unknown value type given to repr_value.");
		break;
	}
}

static void disassemble_constant_table(struct chunk *chunk)
{
	int32_t i;

	printf("constants:\n");
	for (i = 0; i < chunk->constants.length; i++) {
		printf("%04d : ", i);
		print_type(chunk->constants.values[i]);
		printf(" ");
#ifdef DEBUG_CONST_TABLE_EXTRA
		repr_value(chunk->constants.values[i]);
#else
		printf("'");
		print_value(chunk->constants.values[i]);
		printf("'");
#endif
		printf("\n");
	}
}

void disassemble_chunk(struct chunk *chunk, char *name)
{
	int32_t offset;

	printf("== %s ==\n", name);
	disassemble_constant_table(chunk);

	printf("op_code:\n");
	for (offset = 0; offset < chunk->length;)
		offset = disassemble_instruction(chunk, offset);
}

static int32_t simple_instruction(char *name, int32_t offset)
{
	printf("%s\n", name);
	return offset + 1;
}

static int32_t numbered_instruction(char *name, struct chunk *chunk,
				    int32_t offset)
{
	printf("%-16s %4d\n", name, chunk->code[offset + 1]);
	return offset + 2;
}

static int32_t constant_instruction(char *name, struct chunk *chunk,
				    int32_t offset)
{
	uint8_t const_addr = chunk->code[offset + 1];
	printf("%-16s %4d '", name, const_addr);
	print_value(chunk->constants.values[const_addr]);
	printf("'\n");
	return offset + 2;
}

static int32_t long_constant_instruction(char *name, struct chunk *chunk,
					 int32_t offset)
{
	uint8_t const_addr1 = chunk->code[offset + 1],
		const_addr2 = chunk->code[offset + 2],
		const_addr3 = chunk->code[offset + 3];
	int32_t const_addr =
		(const_addr1 << 16) + (const_addr2 << 8) + const_addr3;
	printf("%-16s %4d '", name, const_addr);
	print_value(chunk->constants.values[const_addr]);
	printf("'\n");
	return offset + 4;
}

static int32_t byte_instruction(char *name, struct chunk *chunk, int32_t offset)
{
	uint8_t slot = chunk->code[offset + 1];
	printf("%-16s %4d\n", name, slot);
	return offset + 2;
}

static int32_t jump_instruction(char *name, int32_t sign, struct chunk *chunk,
				int32_t offset)
{
	uint16_t jump = (uint16_t)(chunk->code[offset + 1]) << 8;
	jump |= chunk->code[offset + 2];
	printf("%-16s %4d -> %d\n", name, offset, offset + 3 + jump * sign);
	return offset + 3;
}

int32_t disassemble_instruction(struct chunk *chunk, int32_t offset)
{
	uint8_t instruction;
	int32_t line = read_line(&chunk->lines, offset);

	printf("%04d ", offset);
	if (offset > 0 && line == read_line(&chunk->lines, offset - 1))
		printf("   | ");
	else
		printf("%4d ", line);

	instruction = chunk->code[offset];
	switch (instruction) {
	case OP_CONSTANT:
		return constant_instruction("OP_CONSTANT", chunk, offset);
	case OP_CONSTANT_LONG:
		return long_constant_instruction("OP_CONSTANT_LONG", chunk,
						 offset);
	case OP_NIL:
		return simple_instruction("OP_NIL", offset);
	case OP_TRUE:
		return simple_instruction("OP_TRUE", offset);
	case OP_FALSE:
		return simple_instruction("OP_FALSE", offset);
	case OP_NOT:
		return simple_instruction("OP_NOT", offset);
	case OP_NEGATE:
		return simple_instruction("OP_NEGATE", offset);
	case OP_ADD:
		return simple_instruction("OP_ADD", offset);
	case OP_SUB:
		return simple_instruction("OP_SUB", offset);
	case OP_MUL:
		return simple_instruction("OP_MUL", offset);
	case OP_DIV:
		return simple_instruction("OP_DIV", offset);
	case OP_EQUAL:
		return simple_instruction("OP_EQUAL", offset);
	case OP_LESS:
		return simple_instruction("OP_LESS", offset);
	case OP_GREATER:
		return simple_instruction("OP_GREATER", offset);
	case OP_PRINT:
		return simple_instruction("OP_PRINT", offset);
	case OP_POP:
		return simple_instruction("OP_POP", offset);
	case OP_POPN:
		return numbered_instruction("OP_POPN", chunk, offset);
	case OP_DEFINE_GLOBAL:
		return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
	case OP_DEFINE_GLOBAL_LONG:
		return long_constant_instruction("OP_DEFINE_GLOBAL_LONG", chunk,
						 offset);
	case OP_GET_GLOBAL:
		return constant_instruction("OP_GET_GLOBAL", chunk, offset);
	case OP_GET_GLOBAL_LONG:
		return long_constant_instruction("OP_GET_GLOBAL_LONG", chunk,
						 offset);
	case OP_SET_GLOBAL:
		return constant_instruction("OP_SET_GLOBAL", chunk, offset);
	case OP_SET_GLOBAL_LONG:
		return long_constant_instruction("OP_SET_GLOBAL_LONG", chunk,
						 offset);
	case OP_GET_LOCAL:
		return byte_instruction("OP_GET_LOCAL", chunk, offset);
	case OP_SET_LOCAL:
		return byte_instruction("OP_SET_LOCAL", chunk, offset);
	case OP_JUMP_IF_FALSE:
		return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
	case OP_JUMP:
		return jump_instruction("OP_JUMP", 1, chunk, offset);
	case OP_LOOP:
		return jump_instruction("OP_LOOP", -1, chunk, offset);
	case OP_RETURN:
		return simple_instruction("OP_RETURN", offset);
	default:
		printf("unknown instruction %d\n", instruction);
		return offset + 1;
	}
}
