#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "object.h"
#include "value.h"

struct entry {
	struct object_string *key;
	value_t value;
};

struct table {
	int32_t count;
	int32_t capacity;
	struct entry *entries;
};

#define TABLE_MAX_LOAD 0.75

void init_table(struct table *table);
void free_table(struct table *table);

struct entry *find_entry(struct entry *entries, int32_t capacity,
			 struct object_string *key);
bool table_set(struct table *table, struct object_string *key, value_t value);
bool table_get(struct table *table, struct object_string *key, value_t *value);
void table_add_all(struct table *dest, struct table *src);
bool table_delete(struct table *table, struct object_string *key);
struct object_string *table_find_string(struct table *table, const char *str,
					int32_t length, uint32_t hash);

#endif
