#ifndef __OS_H__
#define __OS_H__

#include <types.h>

i8   *resource_path(i8 *dir, i8 *name);
void  os_setup(void);
void *glfw_func(i8 *name);
void  os_cleanup(void);

#endif/*__OS_H__*/
