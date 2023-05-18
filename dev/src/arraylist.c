#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arraylist.h>

struct arraylist_header {
	u32 size;
	u32 allc;
	u32 type;
};

void *
arraylist_alloc(u32 type_size) {
	struct arraylist_header *header = malloc(sizeof (struct arraylist_header) + type_size);
	if (!header) {
		fprintf(stderr, "error: couldn't allocate memory for the arraylist\n");
		exit(1);
	}
	header->size = 0;
	header->allc = 1;
	header->type = type_size;
	return header + 1;
}

void *
arraylist_push(void *list, void *value) {
	struct arraylist_header *header = ((struct arraylist_header *)list) - 1;
	if (header->allc <= header->size) {
		header->allc *= 2;
		header = realloc(header, sizeof (struct arraylist_header) + header->type * header->allc);
	}
	list = header + 1;
	memcpy(list + header->size * header->type, value, header->type);
	header->size++;
	return list;
}

void
arraylist_pop(void *list, void *out) {
	struct arraylist_header *header = ((struct arraylist_header *)list) - 1;
	header->size--;
	if (!out) return;
	memcpy(out, ((u8 *)list) + header->size * header->type, header->type);
}

u32
arraylist_size(void *list) {
	return (((struct arraylist_header *)list) - 1)->size;
}

void
arraylist_free(void *list) {
	free(((struct arraylist_header *)list) - 1);
}
