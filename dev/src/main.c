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
	os_setup();
	cpu_startup();
	if (!assemble("program.s")) {
    os_cleanup();
    return 1;
  }
	if (!window_open()) {
    os_cleanup();
    return 1;
  }
  window_loop();
	window_close();
	os_cleanup();
	return 0;
}
