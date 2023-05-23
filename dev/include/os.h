#ifndef __OS_H__
#define __OS_H__

#include <types.h>

i8   *resource_path(const i8 *dir, const i8 *name);
void  os_setup(void);
void *glfw_func(const i8 *name);
void  os_framerate(u64 target_framerate);
void  os_frame_begin(void);
void  os_frame_end(void);
void  os_cleanup(void);

#endif/*__OS_H__*/
