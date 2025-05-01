#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, char *argv[])
{
	struct chunk chunk;

	init_vm();

	init_chunk(&chunk);

	write_constant(&chunk, 1.2, 123);
	write_constant(&chunk, 3.4, 123);
	write_chunk(&chunk, OP_ADD, 123);
	write_constant(&chunk, 9.2, 123);
	write_chunk(&chunk, OP_DIV, 123);
	write_chunk(&chunk, OP_NEGATE, 123);
	write_chunk(&chunk, OP_RETURN, 123);

	//disassemble_chunk(&chunk, "test chunk");
	interpret(&chunk);

	free_vm();
	free_chunk(&chunk);

	return 0;
}
