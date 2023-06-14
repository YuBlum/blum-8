#include <os.h>
#include <cpu.h>
#include <renderer.h>
#include <glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <config.h>
#include <window.h>
#include <pthread.h>
#include <assembler.h>

static void *window;
static struct glfw glfw;

static b8
window_center(void) {
  void *monitor = glfw.get_primary_monitor();
  if (!monitor) {
    const i8 *error;
    glfw.get_error(&error);
    fprintf(stderr, "error: could not get primary monitor: %s\n", error);
    return 0;
  }
  struct { i32 width, height; } *vidmode = glfw.get_video_mode(monitor);
  glfw.set_window_pos(window, vidmode->width * 0.5f - WINDOW_SIZE * 0.5f, vidmode->height * 0.5f - WINDOW_SIZE * 0.5f);
  return 1;
}

b8
window_key_get(u32 key) {
  return glfw.get_key(window, key) == GLFW_PRESS;
}

b8
window_open(void) {
  glfw = os_glfw_get();
  if (!glfw.init()) {
    const i8 *error;
    glfw.get_error(&error);
    fprintf(stderr, "error: could not initialize glfw: %s\n", error);
    return 0;
  }
  glfw.window_hint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfw.window_hint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfw.window_hint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfw.window_hint(GLFW_RESIZABLE, 0);
  window = glfw.create_window(WINDOW_SIZE, WINDOW_SIZE, "blum-8", NULL, NULL);
  if (!window) {
    const i8 *error;
    glfw.get_error(&error);
    glfw.terminate();
    fprintf(stderr, "error: could not create window: %s\n", error);
    return 0;
  }
  glfw.make_context_current(window);
  glfw.swap_interval(0); /* deactivate vsync */
  if (!window_center() || !renderer_begin(&glfw)) {
    glfw.terminate();
    return 0;
  }
  os_framerate(60);
  return 1;
}

void
window_loop(void) {
  while (!glfw.window_should_close(window)) {
    os_frame_begin();
    for (u32 i = 0; i < 128*128; i++) {
      cpu_tick();
      cpu_rsu_tick();
    }
    renderer_update();
    glfw.poll_events();
    glfw.swap_buffers(window);
    os_frame_end();
  }
}

void
window_close(void) {
  renderer_end();
  glfw.terminate();
}
