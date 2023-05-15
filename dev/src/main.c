#include <os.h>
#include <stdio.h>
#include <window.h>
#include <bus.h>
#include <cpu.h>

int
main(void) {
	cpu_startup();
	while (1) {
		cpu_clock();
		getchar();
	};
	/*
	os_setup();
	window_open();
	os_cleanup();
	*/
	return 0;
}
