#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile(char *source)
{
	int32_t line = -1;
	struct token token;

	init_scanner(source);

	for (;;) {
		token = scan_token();
		if (token.line != line) {
			printf("%4d ", token.line);
			line = token.line;
		} else {
			printf("   | ");
		}
		printf("%2d '%.*s'\n", token.token_type, token.length,
		       token.start);

		if (token.token_type == TOKEN_EOF)
			break;
	}
}
