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
	os_setup();
	window_open();
	os_cleanup();
	return 0;
}
