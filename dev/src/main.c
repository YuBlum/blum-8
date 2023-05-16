#include <os.h>
#include <bus.h>
#include <cpu.h>
#include <stdio.h>
#include <window.h>
#include <assembler.h>
#include <arraylist.h>

int
main(void) {

	u32 *numbers = arraylist_alloc(sizeof (u32));
	u32 tmp = 2;
	numbers = arraylist_push(numbers, &tmp);
	tmp = 8;
	numbers = arraylist_push(numbers, &tmp);
	tmp = 19;
	numbers = arraylist_push(numbers, &tmp);
	tmp = 20;
	numbers = arraylist_push(numbers, &tmp);
	tmp = 4;
	numbers = arraylist_push(numbers, &tmp);
	arraylist_pop(numbers, &tmp);
	printf("popped %u from numbers\n", tmp);
	arraylist_pop(numbers, &tmp);
	printf("popped %u from numbers\n", tmp);
	for (u32 i = 0; i < arraylist_size(numbers); i++) {
		printf("numbers[%u] = %u\n", i, numbers[i]);
	}
	arraylist_free(numbers);




	/*
	assembler_lex("program.s");
	cpu_startup();
	cpu_interrupt(START_VECTOR);
	while (1) {
		cpu_clock();
		getchar();
	};
	os_setup();
	window_open();
	os_cleanup();
	*/
	return 0;
}
