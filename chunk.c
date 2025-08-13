#include <stdint.h>
#include <stdio.h>

#include "chunk.h"
#include "memory.h"

/**
 * NOTE: This function does no allocation.
 */
void init_line_array(struct line_array *line_array)
{
	line_array->length = 0;
	line_array->capacity = 0;
	line_array->lines = NULL;
}

/**
 * NOTE: The @line_array must be properly initialized before calling this function.
 */
void write_line_array(struct line_array *line_array, int32_t line)
{
	int32_t len = line_array->length, old_capacity;

	if (len > 0 && line_array->lines[len - 1].line == line) {
		line_array->lines[len - 1].run++;
		return;
	}

	if (line_array->capacity < len + 1) {
		old_capacity = line_array->capacity;
		line_array->capacity = GROW_CAPACITY(old_capacity);
		line_array->lines = GROW_ARRAY(struct line_info,
					       line_array->lines, old_capacity,
					       line_array->capacity);
	}

	line_array->lines[len] = (struct line_info){ .line = line, .run = 1 };
	line_array->length++;
}

int32_t read_line(struct line_array *line_array, int32_t instruction)
{
	// Do not worry about time complexity. This function will be only
	// called when there is an error in user's code
	int32_t counter, i;

	for (counter = 0, i = 0; i < line_array->length; i++) {
		counter += line_array->lines[i].run;
		if (instruction < counter)
			return line_array->lines[i].line;
	}

	return -1;
}

void free_line_array(struct line_array *line_array)
{
	FREE_ARRAY(struct line_info, line_array->lines, line_array->capacity);
	init_line_array(line_array);
}

/**
 * NOTE: This function does no allocation.
 */
void init_chunk(struct chunk *chunk)
{
	chunk->length = 0;
	chunk->capacity = 0;
	chunk->code = NULL;
	init_line_array(&chunk->lines);
	init_value_array(&chunk->constants);
}

/**
 * NOTE: The @chunk must be properly initialized before calling this function.
 */
void write_chunk(struct chunk *chunk, uint8_t byte, int32_t line)
{
	int32_t old_capacity;

	if (chunk->capacity < chunk->length + 1) {
		old_capacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(old_capacity);
		chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity,
					 chunk->capacity);
	}

	chunk->code[chunk->length] = byte;
	chunk->length++;

	write_line_array(&chunk->lines, line);
}

int32_t add_constant(struct chunk *chunk, value_t value)
{
	write_value_array(&chunk->constants, value);
	return chunk->constants.length - 1;
}

/**
 * write_constant() - Write constant to a chunk with appropriate opcode.
 * @chunk: Pointer to the chunk where the constant to be written.
 * @opcode: Opcode which will be used to reference the constant.
 * @value: Constant to be written.
 * @line: Line number of the constant.
 *
 * This function attempts to write @value to @chunk's constant table. If there
 * is already more than 256 constants in the table, @opcode will become its long
 * form.
 *
 * NOTE: If there are 2^24 constants written in @chunk, the program exits with a
 * status of 1.
 *
 * NOTE: @opcode must be OP_CONSTANT or OP_CLOSURE.
 */
void write_constant(struct chunk *chunk, uint8_t opcode, value_t value,
		    int32_t line)
{
	int32_t constant;

	constant = add_constant(chunk, value);
	if (constant < (1 << 8)) {
		write_chunk(chunk, opcode, line);
		write_chunk(chunk, constant, line);
	} else if (constant < (1 << 24)) {
		write_chunk(chunk, opcode + 1, line);
		// Big endian
		write_chunk(chunk, (constant >> 16) & 0xFF, line);
		write_chunk(chunk, (constant >> 8) & 0xFF, line);
		write_chunk(chunk, constant & 0xFF, line);
	} else {
		printf("Too many constants.");
		exit(1);
	}
}

void free_chunk(struct chunk *chunk)
{
	FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
	free_value_array(&chunk->constants);
	free_line_array(&chunk->lines);
	init_chunk(chunk);
}
