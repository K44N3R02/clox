#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

void disassemble_chunk(struct chunk *chunk, char *name);
int32_t disassemble_instruction(struct chunk *chunk, int32_t offset);

#endif
