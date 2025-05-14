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
	ss = malloc(sizeof(struct s) + 9 * sizeof(char));
	ss->p = &x;
	ss->x = x;
	strcpy(ss->str, "jkjkjkjk");
	printf("int: %ld byte\nchar: %ld byte\ndouble: %ld byte\nsizeof struct s: %ld\noffset: %ld\n",
	       sizeof(int), sizeof(char), sizeof(double), sizeof(struct s),
	       offsetof(struct s, str));
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
	for (ptr = ss, i = 0; i < sizeof(struct s); i++)
		printf("%d\n", *(ptr + i));
#pragma clang diagnostic pop
	printf("%s\n", ss->str);
	return 0;
}
