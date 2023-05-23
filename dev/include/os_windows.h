#include <windows.h>
#include <stdio.h>

static i8    abs_path[MAX_PATH + 1];
static i8    tmp_path[MAX_PATH + 129];
static void *glfw;
static i64   start_time;
static i64   end_time;
static i64   framerate;
static i64   frequency;

i8 *
resource_path(const i8 *dir, const i8 *name) {
	snprintf(tmp_path, MAX_PATH + 128, "%s%s\\%s", abs_path, dir, name);
	return tmp_path;
}

void
os_setup(void) {
	i32 len = GetModuleFileName(NULL, abs_path, MAX_PATH);
	abs_path[len] = '\0';
	for (i32 i = len - 1; i >= 0; i--) {
		if (abs_path[i] != '\\') continue;
		abs_path[i + 1] = '\0';
		break;
	}
	glfw = LoadLibrary(resource_path("libs", "glfw3.dll"));
	if (!glfw) {
		fprintf(stderr, "error: glfw not found\n");
		exit(1);
	}
	timeBeginPeriod(1);
}

void *
glfw_func(const i8 *name) {
	void *func = GetProcAddress(glfw, name);
	if (!func) {
		fprintf(stderr, "error: could not find the function '%s'\n", name);
		exit(1);
	}
	return func;
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
	FreeLibrary(glfw);
	timeEndPeriod(1);
}
