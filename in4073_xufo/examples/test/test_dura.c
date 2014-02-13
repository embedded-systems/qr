#include <x32.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

void test_math();
void test_comp();
void test_array_struct();
void test_lib_string();
void test_lib_std_setjmp();
void test_lib_io();
void test_alu_errors();
void test_trap();
void test_oom();

int running;

unsigned timer1_low, timer1_high;
unsigned timer2_low, timer2_high;
unsigned switches_low, switches_high;
unsigned buttons_low, buttons_high;
unsigned rx_low, rx_high;
unsigned tests_low, tests_high;

void isr_timer1() {
	timer1_low++;
	peripherals[PERIPHERAL_TIMER1_PERIOD] = (rand()%9+1)*CLOCKS_PER_MS;
	if (timer1_low == 0) timer1_high++;
}

void isr_timer2() {
	timer2_low++;
	peripherals[PERIPHERAL_TIMER2_PERIOD] = (rand()%19+1)*CLOCKS_PER_MS;
	if (timer2_low == 0) timer2_high++;
}

void isr_buttons() {
	buttons_low++;
	putchar('*');
	if (buttons_low == 0) buttons_high++;
}

void isr_switches() {
	switches_low++;
	putchar('#');
	if (switches_low == 0) switches_high++;
}

void isr_rx() {
	int c;
	while ((c = getchar_nb()) != -1) {
		if (c == 0x1B) running = 0;
		if (isprint(c)) putchar(c);
	}
	rx_low++;
	if (rx_low == 0) rx_high++;
}

int main() {
	INTERRUPT_VECTOR(INTERRUPT_TIMER1) = &isr_timer1;
	INTERRUPT_PRIORITY(INTERRUPT_TIMER1) = 10;
	INTERRUPT_VECTOR(INTERRUPT_TIMER2) = &isr_timer2;
	INTERRUPT_PRIORITY(INTERRUPT_TIMER2) = 10;
	INTERRUPT_VECTOR(INTERRUPT_BUTTONS) = &isr_buttons;
	INTERRUPT_PRIORITY(INTERRUPT_BUTTONS) = 10;
	INTERRUPT_VECTOR(INTERRUPT_SWITCHES) = &isr_switches;
	INTERRUPT_PRIORITY(INTERRUPT_SWITCHES) = 10;
	INTERRUPT_VECTOR(INTERRUPT_PRIMARY_RX) = &isr_rx;
	INTERRUPT_PRIORITY(INTERRUPT_PRIMARY_RX) = 20;

	peripherals[PERIPHERAL_TIMER1_PERIOD] = 1*CLOCKS_PER_MS;
	peripherals[PERIPHERAL_TIMER2_PERIOD] = 2*CLOCKS_PER_MS;

	timer1_low = timer1_high = 0;
	timer2_low = timer2_high = 0;
	switches_low = switches_high = 0;
	buttons_low = buttons_high = 0;
	rx_low = rx_high = 0;
	tests_low = tests_high = 0;
	
	srand(X32_MS_CLOCK);

	ENABLE_INTERRUPT(INTERRUPT_TIMER1);
	ENABLE_INTERRUPT(INTERRUPT_TIMER2);
	ENABLE_INTERRUPT(INTERRUPT_BUTTONS);
	ENABLE_INTERRUPT(INTERRUPT_SWITCHES);
	ENABLE_INTERRUPT(INTERRUPT_PRIMARY_RX);
	ENABLE_INTERRUPT(INTERRUPT_GLOBAL);
	puts("Running dura-tests, press [ESC] to exit");

	running = 1;
	while(running) {
		test_math();
		test_comp();
		test_array_struct();
		test_lib_string();
		// the alu overflow can't be tested, as several interrupts cause overflow,
		//  which causes the alu_errors to detect overflows on the wrong statements
		//test_alu_errors();
		test_lib_std_setjmp();
		test_lib_io();
		test_trap();
		test_oom();
		tests_low++;
		if (tests_low == 0) tests_high++;
	}

	DISABLE_INTERRUPT(INTERRUPT_TIMER1);
	DISABLE_INTERRUPT(INTERRUPT_TIMER2);
	DISABLE_INTERRUPT(INTERRUPT_BUTTONS);
	DISABLE_INTERRUPT(INTERRUPT_SWITCHES);
	DISABLE_INTERRUPT(INTERRUPT_PRIMARY_RX);
	DISABLE_INTERRUPT(INTERRUPT_GLOBAL);

	puts("\r\nDone");

	printf("Nr of tests run:     0x%08X%08X\r\n", tests_high, tests_low);
	printf("\r\n");
	printf("Timer1 interrupts:   0x%08X%08X\r\n", timer1_high, timer1_low);
	printf("Timer2 interrupts:   0x%08X%08X\r\n", timer2_high, timer2_low);
	printf("Button interrupts:   0x%08X%08X\r\n", buttons_high, buttons_low);
	printf("Switches interrupts: 0x%08X%08X\r\n", switches_high, switches_low);
	printf("RS232 RX interrupts: 0x%08X%08X\r\n", rx_high, rx_low);

	return 0;
}
