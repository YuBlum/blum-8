#ifndef __ARRAYLIST_H__
#define __ARRAYLIST_H__

#include <types.h>

void *arraylist_alloc(u32 type_size);
void *arraylist_push(void *list, void *value);
void  arraylist_pop(void  *list, void *out);
u32   arraylist_size(void *list);
void  arraylist_free(void *list);

#endif/*__ARRAYLIST_H__*/
