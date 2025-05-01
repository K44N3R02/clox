#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, char *argv[])
{
	int i;
	struct chunk chunk;

	init_chunk(&chunk);

	for (i = 0; i < 300; i++)
		write_constant(&chunk, (double)i * i, i + 1);
	write_chunk(&chunk, OP_RETURN, i + 1);

	disassemble_chunk(&chunk, "test chunk");

	free_chunk(&chunk);

	return 0;
}
