#include <os.h>
#include <stdio.h>
#include <window.h>

int
main(void) {
	os_setup();
	window_open();
	os_cleanup();
	return 0;
}
