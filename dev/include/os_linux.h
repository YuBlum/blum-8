#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>

static i8    abs_path[MAX_PATH + 1];
static i8    tmp_path[MAX_PATH + 129];
static void *glfw;

i8 *
resource_path(i8 *dir, i8 *name) {
	snprintf(tmp_path, MAX_PATH + 128, "%s%s/%s", abs_path, dir, name);
	return tmp_path;
}

void
os_setup(void) {
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
	glfw = dlopen(resource_path("libs", "libglfw.so"), RTLD_LAZY);
	if (!glfw) {
		fprintf(stderr, "error: glfw not found\n");
		exit(1);
	}
}

void *
glfw_func(i8 *name) {
	void *func = dlsym(glfw, name);
	if (!func) {
		fprintf(stderr, "error: could not find the function '%s'\n", name);
		exit(1);
	}
	return func;
}

void
os_cleanup(void) {
	dlclose(glfw);
}
