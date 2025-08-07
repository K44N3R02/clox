#ifndef clox_compiler_h
#define clox_compiler_h

#include "common.h"
#include "chunk.h"
#include "object.h"
#include "vm.h"

struct object_function *compile(char *source);

#endif
