#ifndef __OS_H__
#define __OS_H__

#include <types.h>

struct glfw {
  i32   (*init)(void);
  void  (*terminate)(void);
  i32   (*get_error)(const i8 **);
  void  (*window_hint)(i32, i32);
  void *(*create_window)(i32, i32, const i8 *, void *, void *);
  i32   (*window_should_close)(void *);
  void  (*make_context_current)(void *);
  void  (*poll_events)(void);
  void  (*swap_buffers)(void *);
  void *(*get_primary_monitor)(void);
  void *(*get_video_mode)(void *);
  void *(*set_window_pos)(void *, i32, i32);
  void  (*swap_interval)(i32);
	void *(*get_proc_address)(const i8 *);
};

i8          *resource_path(const i8 *dir, const i8 *name);
struct glfw  os_glfw_get(void);
void         os_setup(void);
void         os_framerate(u64 target_framerate);
void         os_frame_begin(void);
void         os_frame_end(void);
void         os_cleanup(void);

#undef MAX_PATH
#define MAX_PATH 1028

#endif/*__OS_H__*/
