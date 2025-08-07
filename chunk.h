#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

enum op_code {
	OP_CONSTANT,
	OP_CONSTANT_LONG,
	OP_NIL,
	OP_TRUE,
	OP_FALSE,
	OP_NOT,
	OP_NEGATE,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_EQUAL,
	OP_LESS,
	OP_GREATER,
	OP_PRINT,
	OP_POP,
	OP_POPN,
	OP_DEFINE_GLOBAL,
	OP_DEFINE_GLOBAL_LONG,
	OP_GET_GLOBAL,
	OP_GET_GLOBAL_LONG,
	OP_SET_GLOBAL,
	OP_SET_GLOBAL_LONG,
	OP_GET_LOCAL,
	OP_SET_LOCAL,
	OP_JUMP_IF_FALSE,
	OP_JUMP,
	OP_LOOP,
	OP_CALL,
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
int32_t add_constant(struct chunk *chunk, value_t value);
void write_constant(struct chunk *chunk, value_t value, int32_t line);
void free_chunk(struct chunk *chunk);

#endif
