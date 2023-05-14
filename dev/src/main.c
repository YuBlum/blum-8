#include <os.h>
#include <stdio.h>
#include <window.h>
#include <bus.h>

int
main(void) {
	bus_initialize();
	os_setup();
	window_open();
	os_cleanup();
	return 0;
}
