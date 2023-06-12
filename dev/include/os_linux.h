#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <time.h>
#include <types.h>
#include <os.h>

static i8              abs_path[MAX_PATH + 1];
static i8              tmp_path[MAX_PATH + 129];
static void           *glfw_lib;
static u64             framerate;
static struct timespec start_time;
static struct timespec end_time;
static struct glfw     glfw;

i8 *
resource_path(const i8 *dir, const i8 *name) {
	snprintf(tmp_path, MAX_PATH + 128, "%s%s/%s", abs_path, dir, name);
	return tmp_path;
}

static void
setup_resource_path(void) {
	i64 len = readlink("/proc/self/exe", abs_path, MAX_PATH);
	if (len < 0) {
		fprintf(stderr, "error: could not get the executable path: %s\n", strerror(errno));
		exit(1);
	}
	abs_path[len] = '\0';
	for (i32 i = len - 1; i >= 0; i--) {
		if (abs_path[i] != '/') continue;
		abs_path[i + 1] = '\0';
		break;
	}
}

static void *
lib_load_func(void *library, const i8 *name) {
	void *func = dlsym(library, name);
	if (!func) {
		fprintf(stderr, "error: could not find the function '%s'\n", name);
    os_cleanup();
		exit(1);
	}
	return func;
}

static void
glfw_load() {
	glfw_lib = dlopen(resource_path("libs", "libglfw.so"), RTLD_LAZY);
	if (!glfw_lib) {
		fprintf(stderr, "error: glfw not found\n");
    os_cleanup();
		exit(1);
	}
  glfw.init                 = lib_load_func(glfw_lib, "glfwInit");
  glfw.get_error            = lib_load_func(glfw_lib, "glfwGetError");
  glfw.window_hint          = lib_load_func(glfw_lib, "glfwWindowHint");
  glfw.create_window        = lib_load_func(glfw_lib, "glfwCreateWindow");
  glfw.window_should_close  = lib_load_func(glfw_lib, "glfwWindowShouldClose");
  glfw.make_context_current = lib_load_func(glfw_lib, "glfwMakeContextCurrent");
  glfw.poll_events          = lib_load_func(glfw_lib, "glfwPollEvents");
  glfw.swap_buffers         = lib_load_func(glfw_lib, "glfwSwapBuffers");
  glfw.get_primary_monitor  = lib_load_func(glfw_lib, "glfwGetPrimaryMonitor");
  glfw.get_video_mode       = lib_load_func(glfw_lib, "glfwGetVideoMode");
  glfw.set_window_pos       = lib_load_func(glfw_lib, "glfwSetWindowPos");
  glfw.swap_interval        = lib_load_func(glfw_lib, "glfwSwapInterval");
	glfw.get_proc_address     = lib_load_func(glfw_lib, "glfwGetProcAddress");
	glfw.terminate            = lib_load_func(glfw_lib, "glfwTerminate");
}

void
os_setup(void) {
  setup_resource_path();
  glfw_load();
}

struct glfw
os_glfw_get(void) {
  return glfw;
}

void
os_framerate(u64 target_framerate) {
	framerate = 1e9 / target_framerate;
}

void
os_frame_begin(void) {
	clock_gettime(CLOCK_MONOTONIC, &start_time);
}

void
os_frame_end(void) {
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	u64 elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1e9 + (end_time.tv_nsec - start_time.tv_nsec);
	if (elapsed_time < framerate) {
		nanosleep(&(struct timespec) { 0, framerate - elapsed_time }, NULL);
	}
	clock_gettime(CLOCK_MONOTONIC, &end_time);
}

void
os_cleanup(void) {
	if (glfw_lib) dlclose(glfw_lib);
}
