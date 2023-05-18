#include <os.h>
#include <bus.h>
#include <cpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <window.h>
#include <assembler.h>
#include <arraylist.h>

int
main(void) {
	cpu_startup();
	if (assemble("program.s")) exit(1);
	cpu_interrupt(RESET_VECTOR);
	/*
	while (1) {
		cpu_clock();
		getchar();
	};
	*/
	os_setup();
	window_open();
	os_cleanup();
	return 0;
}
