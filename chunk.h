#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

enum op_code {
	OP_CONSTANT,
	OP_CONSTANT_LONG,
	OP_RETURN,
};

struct line_info {
	int32_t line;
	int32_t run;
};

struct line_array {
	int32_t length;
	int32_t capacity;
	struct line_info *lines;
};

void init_line_array(struct line_array *line_array);
void write_line_array(struct line_array *line_array, int32_t line);
int32_t read_line(struct line_array *line_array, int32_t instruction);
void free_line_array(struct line_array *line_array);

struct chunk {
	int32_t length;
	int32_t capacity;
	uint8_t *code;
	struct line_array lines;
	struct value_array constants;
};

void init_chunk(struct chunk *chunk);
void write_chunk(struct chunk *chunk, uint8_t byte, int32_t line);
void write_constant(struct chunk *chunk, value_t value, int32_t line);
void free_chunk(struct chunk *chunk);

#endif
