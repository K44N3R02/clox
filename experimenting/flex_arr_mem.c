#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct s {
	int *p;
	int x;
	char str[];
};

int main(void)
{
	char *ptr;
	unsigned long i;
	int x = 123;
	struct s *ss;
	ss = malloc(offsetof(struct s, str) + 9 * sizeof(char));
	ss->p = &x;
	ss->x = x;
	memcpy(ss->str, "jkjkjkjk", 8);
	printf("int: %ld byte\nchar: %ld byte\ndouble: %ld byte\nsizeof struct s: %ld\noffset: %ld\n",
	       sizeof(int), sizeof(char), sizeof(double), sizeof(struct s),
	       offsetof(struct s, str));
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
	for (ptr = ss, i = 0; i < sizeof(struct s) + 9 * sizeof(char); i++) {
		printf("%d\n", *(ptr + i));
		*(ptr + i) = 'a';
	}
#pragma clang diagnostic pop
	printf("%s\n", ss->str);
	free(ss);
	return 0;
}
