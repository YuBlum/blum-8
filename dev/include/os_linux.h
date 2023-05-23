#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <time.h>

static i8              abs_path[MAX_PATH + 1];
static i8              tmp_path[MAX_PATH + 129];
static void           *glfw;
static u64             framerate;
static struct timespec start_time;
static struct timespec end_time;

i8 *
resource_path(const i8 *dir, const i8 *name) {
	snprintf(tmp_path, MAX_PATH + 128, "%s%s/%s", abs_path, dir, name);
	return tmp_path;
}

void
os_setup(void) {
	/* setup resources path */
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
	/* get glfw library */
	glfw = dlopen(resource_path("libs", "libglfw.so"), RTLD_LAZY);
	if (!glfw) {
		fprintf(stderr, "error: glfw not found\n");
		exit(1);
	}
}

void *
glfw_func(const i8 *name) {
	void *func = dlsym(glfw, name);
	if (!func) {
		fprintf(stderr, "error: could not find the function '%s'\n", name);
		exit(1);
	}
	return func;
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
	dlclose(glfw);
}
