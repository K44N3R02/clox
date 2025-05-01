#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "value.h"

void disassemble_chunk(struct chunk *chunk, char *name)
{
	int32_t offset;

	printf("== %s ==\n", name);

	for (offset = 0; offset < chunk->length;)
		offset = disassemble_instruction(chunk, offset);
}

static int32_t simple_instruction(char *name, int32_t offset)
{
	printf("%s\n", name);
	return offset + 1;
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
	case OP_RETURN:
		return simple_instruction("OP_RETURN", offset);
	default:
		printf("unknown instruction %d\n", instruction);
		return offset + 1;
	}
}
