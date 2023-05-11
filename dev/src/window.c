#include <renderer.h>
#include <window.h>
#include <glfw3.h>
#include <stdio.h>
#include <os.h>

static void *window;

void
window_open(void) {
	i32   (*glfw_init)(void)                                          = glfw_func("glfwInit");
	void  (*glfw_terminate)(void)                                     = glfw_func("glfwTerminate");
	i32   (*glfw_get_error)(const i8 **)                              = glfw_func("glfwGetError");
	void  (*glfw_window_hint)(i32, i32)                               = glfw_func("glfwWindowHint");
	void *(*glfw_create_window)(i32, i32, const i8 *, void *, void *) = glfw_func("glfwCreateWindow");
	i32   (*glfw_window_should_close)(void *)                         = glfw_func("glfwWindowShouldClose");
	void  (*glfw_make_context_current)(void *)                        = glfw_func("glfwMakeContextCurrent");
	void  (*glfw_poll_events)(void)                                   = glfw_func("glfwPollEvents");
	void  (*glfw_swap_buffers)(void *)                                = glfw_func("glfwSwapBuffers");
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
	window = glfw_create_window(480, 480, "blum-8", NULL, NULL);
	if (!window) {
		const i8 *error;
		glfw_get_error(&error);
		fprintf(stderr, "error: could not create window: %s\n", error);
		return;
	}
	glfw_make_context_current(window);
	renderer_begin();
	while (!glfw_window_should_close(window)) {
		renderer_update();
		glfw_poll_events();
		glfw_swap_buffers(window);
	}
	renderer_end();
	glfw_terminate();
}
