#include <os.h>
#include <cpu.h>
#include <crt.h>
#include <glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <config.h>
#include <window.h>
#include <pthread.h>
#include <assembler.h>

static void *window;
static void  (*glfw_terminate)(void);

void
window_open(void) {
	i32   (*glfw_init)(void)                                          = glfw_func("glfwInit");
	i32   (*glfw_get_error)(const i8 **)                              = glfw_func("glfwGetError");
	void  (*glfw_window_hint)(i32, i32)                               = glfw_func("glfwWindowHint");
	void *(*glfw_create_window)(i32, i32, const i8 *, void *, void *) = glfw_func("glfwCreateWindow");
	i32   (*glfw_window_should_close)(void *)                         = glfw_func("glfwWindowShouldClose");
	void  (*glfw_make_context_current)(void *)                        = glfw_func("glfwMakeContextCurrent");
	void  (*glfw_poll_events)(void)                                   = glfw_func("glfwPollEvents");
	void  (*glfw_swap_buffers)(void *)                                = glfw_func("glfwSwapBuffers");
	void *(*glfw_get_primary_monitor)(void)                           = glfw_func("glfwGetPrimaryMonitor");
	void *(*glfw_get_video_mode)(void *)                              = glfw_func("glfwGetVideoMode");
	void *(*glfw_set_window_pos)(void *, i32, i32)                    = glfw_func("glfwSetWindowPos");
	void  (*glfw_swap_interval)(i32)                                  = glfw_func("glfwSwapInterval");
	/*f64   (*glfw_get_time)(void)                                      = glfw_func("glfwGetTime");*/
	glfw_terminate = glfw_func("glfwTerminate");
	if (!glfw_init()) {
		const i8 *error;
		glfw_get_error(&error);
		fprintf(stderr, "error: could not initialize glfw: %s\n", error);
		return;
	}
	glfw_window_hint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfw_window_hint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfw_window_hint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfw_window_hint(GLFW_RESIZABLE, 0);
	window = glfw_create_window(WINDOW_SIZE, WINDOW_SIZE, "blum-8", NULL, NULL);
	if (!window) {
		const i8 *error;
		glfw_get_error(&error);
		glfw_terminate();
		fprintf(stderr, "error: could not create window: %s\n", error);
		return;
	}
	void *monitor = glfw_get_primary_monitor();
	if (!monitor) {
		const i8 *error;
		glfw_get_error(&error);
		glfw_terminate();
		fprintf(stderr, "error: could not get primary monitor: %s\n", error);
		return;
	}
	struct { i32 width, height; } *vidmode = glfw_get_video_mode(monitor);
	glfw_make_context_current(window);
	glfw_swap_interval(0);
	glfw_set_window_pos(window, vidmode->width * 0.5f - WINDOW_SIZE * 0.5f, vidmode->height * 0.5f - WINDOW_SIZE * 0.5f);
	crt_begin();
	os_framerate(60);
	while (!glfw_window_should_close(window)) {
		os_frame_begin();
		for (u32 i = 0; i < 128; i++) {
			cpu_tick();
		}
		crt_update();
		glfw_poll_events();
		glfw_swap_buffers(window);
		os_frame_end();
	}
}

void
window_close(void) {
	crt_end();
	glfw_terminate();
}
