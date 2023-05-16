#ifndef __OS_H__
#define __OS_H__

#include <types.h>

i8   *resource_path(const i8 *dir, const i8 *name);
void  os_setup(void);
void *glfw_func(const i8 *name);
void  os_cleanup(void);

#endif/*__OS_H__*/
