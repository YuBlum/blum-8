#include <windows.h>
#include <stdio.h>
#include <os.h>

static i8           abs_path[MAX_PATH + 1];
static i8           tmp_path[MAX_PATH + 129];
static i64          start_time;
static i64          end_time;
static i64          framerate;
static i64          frequency;
static void        *glfw_lib;
static struct glfw  glfw;

i8 *
resource_path(const i8 *dir, const i8 *name) {
	snprintf(tmp_path, MAX_PATH + 128, "%s%s\\%s", abs_path, dir, name);
	return tmp_path;
}

static void
setup_resource_path(void) {
	i32 len = GetModuleFileName(NULL, abs_path, MAX_PATH);
	abs_path[len] = '\0';
	for (i32 i = len - 1; i >= 0; i--) {
		if (abs_path[i] != '\\') continue;
		abs_path[i + 1] = '\0';
		break;
	}
}

static void *
lib_load_func(void *library, const i8 *name) {
	void *func = GetProcAddress(library, name);
	if (!func) {
		fprintf(stderr, "error: could not find the function '%s'\n", name);
    os_cleanup();
		exit(1);
	}
	return func;
}

static void
glfw_load() {
	glfw_lib = LoadLibrary(resource_path("libs", "glfw3.dll"));
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
	timeBeginPeriod(1);
  setup_resource_path();
  glfw_load();
}

struct glfw
os_glfw_get(void) {
  return glfw;
}

void
os_framerate(u64 target_framerate) {
	QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
	framerate = frequency / target_framerate;
}

void
os_frame_begin(void) {
	QueryPerformanceCounter((LARGE_INTEGER *)&start_time);
}

void
os_frame_end(void) {
	QueryPerformanceCounter((LARGE_INTEGER *)&end_time);
	i64 elapsed_time = end_time - start_time;
	if (elapsed_time < framerate) {
		Sleep((f64)(framerate - elapsed_time) / frequency * 1000);
	}
	do {
		Sleep(0);
		QueryPerformanceCounter((LARGE_INTEGER *)&end_time);
		elapsed_time = end_time - start_time;
	} while (elapsed_time < framerate);
}

void
os_cleanup(void) {
	if (glfw_lib) FreeLibrary(glfw_lib);
	timeEndPeriod(1);
}
