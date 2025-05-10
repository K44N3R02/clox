#ifndef clox_compiler_h
#define clox_compiler_h

#include "common.h"
#include "chunk.h"
#include "vm.h"

bool compile(char *source, struct chunk *chunk);

#endif
