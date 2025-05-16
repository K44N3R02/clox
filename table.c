#include <string.h>

#include "memory.h"
#include "table.h"

void init_table(struct table *table)
{
	table->count = 0;
	table->capacity = 0;
	table->entries = NULL;
}

void free_table(struct table *table)
{
	FREE_ARRAY(struct entry, table->entries, table->capacity);
	init_table(table);
}

static void adjust_capacity(struct table *table, int32_t capacity)
{
	int32_t i;
	struct entry *entries = ALLOCATE(struct entry, capacity), *entry, *old;

	for (i = 0; i < capacity; i++) {
		entry = entries + i;
		entry->key = NULL;
		entry->value = CONS_NIL;
	}

	table->count = 0;
	for (i = 0; i < table->capacity; i++) {
		old = &table->entries[i];
		if (old->key == NULL)
			continue;
		entry = find_entry(entries, capacity, old->key);
		entry->key = old->key;
		entry->value = old->value;
		table->count++;
	}

	FREE_ARRAY(struct entry, table->entries, table->capacity);
	table->entries = entries;
	table->capacity = capacity;
}

struct entry *find_entry(struct entry *entries, int32_t capacity,
			 struct object_string *key)
{
	struct entry *entry, *tombstone = NULL;
	int32_t bucket = key->hash % capacity;

	for (;;) {
		entry = &entries[bucket];
		if (entry->key == NULL) {
			if (IS_NIL(entry->value)) // not tombstone
				return tombstone == NULL ? entry : tombstone;
			else if (tombstone == NULL)
				tombstone = entry;
		} else if (entry->key == key) {
			return entry;
		}
		bucket = (bucket + 1) % capacity;
	}
}

bool table_set(struct table *table, struct object_string *key, value_t value)
{
	struct entry *bucket;
	bool new_key;

	if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
		int32_t capacity = GROW_CAPACITY(table->capacity);
		adjust_capacity(table, capacity);
	}

	bucket = find_entry(table->entries, table->capacity, key);
	new_key = bucket->key == NULL;

	if (new_key && IS_NIL(bucket->value))
		table->count++;
	bucket->key = key;
	bucket->value = value;

	return new_key;
}

bool table_get(struct table *table, struct object_string *key, value_t *value)
{
	struct entry *entry;

	if (table->count == 0)
		return false;

	entry = find_entry(table->entries, table->capacity, key);
	if (entry->key == NULL)
		return false;

	*value = entry->value;
	return true;
}

void table_add_all(struct table *dest, struct table *src)
{
	int32_t i;
	struct entry *entry;

	for (i = 0; i < src->capacity; i++) {
		entry = &src->entries[i];
		if (entry == NULL)
			continue;
		table_set(dest, entry->key, entry->value);
	}
}

bool table_delete(struct table *table, struct object_string *key)
{
	struct entry *entry;

	if (table->count == 0)
		return false;

	entry = find_entry(table->entries, table->capacity, key);
	if (entry->key == NULL)
		return false;

	entry->key = NULL;
	entry->value = CONS_BOOLEAN(true);
	return true;
}

struct object_string *table_find_string(struct table *table, const char *str,
					int32_t length, uint32_t hash)
{
	struct entry *entry;
	int32_t bucket;

	if (table->count == 0)
		return NULL;

	bucket = hash % table->capacity;
	for (;;) {
		entry = &table->entries[bucket];
		if (entry->key == NULL) {
			if (IS_NIL(entry->value)) // not tombstone
				return NULL;
		} else if (entry->key->length == length &&
			   entry->key->hash == hash &&
			   memcmp(entry->key->characters, str, length) == 0) {
			return entry->key;
		}
		bucket = (bucket + 1) % table->capacity;
	}
}
