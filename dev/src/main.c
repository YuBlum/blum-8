#include <os.h>
#include <bus.h>
#include <cpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <window.h>
#include <assembler.h>
#include <arraylist.h>
#include <time.h>
#include <unistd.h>

int
main(void) {
	cpu_startup();
	if (assemble("program.s")) exit(1);
	os_setup();
	window_open();
	window_close();
	os_cleanup();
	return 0;
}
