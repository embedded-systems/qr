/*
 * This is a small sample program which uses interrupts, and both rs232 
 * connections.
 *
 *
 */
#include <x32.h>
#include <stdlib.h>
#include <stdio.h>

/* interrupt handler prototypes */
void int_timer1(void);
void int_timer2(void);
void int_com1_in(void);
void int_com1_out(void);
void int_com2_in(void);
void int_com2_out(void);
void int_buttons(void);
void int_switches(void);

int quit = 0;

/* main entry point */
int main() {
	int time, iterations;

	/* make the code somewhat faster when running in the sim */
	if (STATE_SIMULATOR) {
		/* set timer 1 period to 5 seconds */
		peripherals[PERIPHERAL_TIMER1_PERIOD] = 100*CLOCKS_PER_MS;
		/* set timer 2 period to 7 seconds */
		peripherals[PERIPHERAL_TIMER2_PERIOD] = 140*CLOCKS_PER_MS;
		printf("Running in simulation mode!\n");
	} else {
		/* set timer 1 period to 5 seconds */
		peripherals[PERIPHERAL_TIMER1_PERIOD] = 5000*CLOCKS_PER_MS;
		/* set timer 2 period to 7 seconds */
		peripherals[PERIPHERAL_TIMER2_PERIOD] = 7000*CLOCKS_PER_MS;
	}
	
	/* set interrupt addresses & priorities */
	/* note: the priority MUST be > 1, larger means higher prio */
	SET_INTERRUPT_VECTOR(INTERRUPT_TIMER1, &int_timer1);
	SET_INTERRUPT_PRIORITY(INTERRUPT_TIMER1, 50);
	SET_INTERRUPT_VECTOR(INTERRUPT_TIMER2, &int_timer2);
	SET_INTERRUPT_PRIORITY(INTERRUPT_TIMER2, 40);
	SET_INTERRUPT_VECTOR(INTERRUPT_PRIMARY_RX, &int_com1_in);
	SET_INTERRUPT_PRIORITY(INTERRUPT_PRIMARY_RX, 30);
	SET_INTERRUPT_VECTOR(INTERRUPT_PRIMARY_TX, &int_com1_out);
	SET_INTERRUPT_PRIORITY(INTERRUPT_PRIMARY_TX, 20);
#ifdef INTERRUPT_SECONDARY_RX
	SET_INTERRUPT_VECTOR(INTERRUPT_SECONDARY_RX, &int_com2_in);
	SET_INTERRUPT_PRIORITY(INTERRUPT_SECONDARY_RX, 30);
#endif
#ifdef INTERRUPT_SECONDARY_TX
	SET_INTERRUPT_VECTOR(INTERRUPT_SECONDARY_TX, &int_com2_out);
	SET_INTERRUPT_PRIORITY(INTERRUPT_SECONDARY_TX, 20);
#endif
	SET_INTERRUPT_VECTOR(INTERRUPT_BUTTONS, &int_buttons);
	SET_INTERRUPT_PRIORITY(INTERRUPT_BUTTONS, 10);
	SET_INTERRUPT_VECTOR(INTERRUPT_SWITCHES, &int_switches);
	SET_INTERRUPT_PRIORITY(INTERRUPT_SWITCHES, 10);

	/* enable interrupts */
	ENABLE_INTERRUPT(INTERRUPT_TIMER1);
	ENABLE_INTERRUPT(INTERRUPT_TIMER2);
	ENABLE_INTERRUPT(INTERRUPT_PRIMARY_RX);
	//ENABLE_INTERRUPT(INTERRUPT_PRIMARY_TX);
#ifdef INTERRUPT_SECONDARY_RX
	ENABLE_INTERRUPT(INTERRUPT_SECONDARY_RX);
#endif
#ifdef INTERRUPT_SECONDARY_TX
	//ENABLE_INTERRUPT(INTERRUPT_SECONDARY_TX);
#endif
	ENABLE_INTERRUPT(INTERRUPT_BUTTONS);
	ENABLE_INTERRUPT(INTERRUPT_SWITCHES);
	/* this is the global 'turn all interrupts on/off interrupt',
			when it's off, no interrupt will ever trigger, when it's
			on, all enabled interrupts may trigger */
	ENABLE_INTERRUPT(INTERRUPT_GLOBAL);

	/* run idle loop, print "IDLE LOOP" each 1000 iterations */
	time = iterations = 0;
	while(!quit) {
		if ((STATE_SIMULATOR && X32_MS_CLOCK - time > 50) || X32_MS_CLOCK - time > 1000) {
			printf("IDLE LOOP: %d iterations/sec\r\n", iterations);
			iterations = 0;
			time = X32_MS_CLOCK;
		}
		iterations++;
	}
	
	/* disable interrupts */
	DISABLE_INTERRUPT(INTERRUPT_GLOBAL);
	DISABLE_INTERRUPT(INTERRUPT_TIMER1);
	DISABLE_INTERRUPT(INTERRUPT_TIMER2);
	DISABLE_INTERRUPT(INTERRUPT_PRIMARY_RX);
	DISABLE_INTERRUPT(INTERRUPT_PRIMARY_TX);
#ifdef INTERRUPT_SECONDARY_RX
	DISABLE_INTERRUPT(INTERRUPT_SECONDARY_RX);
#endif
#ifdef INTERRUPT_SECONDARY_TX
	DISABLE_INTERRUPT(INTERRUPT_SECONDARY_TX);
#endif
	DISABLE_INTERRUPT(INTERRUPT_BUTTONS);
	DISABLE_INTERRUPT(INTERRUPT_SWITCHES);

	return 0;
}

void int_timer1(void) {
	printf("Timer 1 tick\r\n");
}

void int_timer2(void) {
	printf("Timer 2 tick\r\n");
}

void int_com1_in(void) {
	int tmp;
	/* handle all bytes, note that the processor will sometimes generate
		* an interrupt while there is no byte available, make sure the handler
		* checks the state of the com channel before fetching a character from
		* the buffer. Also it is recommended to use a while loop to handle all
		* available characters.
		*/
	while (COM_BYTE_AVAILABLE(PERIPHERAL_PRIMARY_STATUS)) {
		tmp = peripherals[PERIPHERAL_PRIMARY_DATA];
		printf("Communication channel 1 Rx: %02X\r\n", tmp);
		if (tmp == 0x1B) quit = 1;
	}
}

void int_com1_out(void) {
	/* don't do anything on this interrupt */
}

void int_com2_in(void) {
	int tmp;
	/* handle all bytes */
#ifdef PERIPHERAL_SECONDARY_STATUS
#ifdef PERIPHERAL_SECONDARY_DATA
	while (COM_BYTE_AVAILABLE(PERIPHERAL_SECONDARY_STATUS)) {
		tmp = peripherals[PERIPHERAL_SECONDARY_DATA];
		// echo channel 2
		// note: the output buffer should be monitored here, to see
		//		if there is room for outgoing bytes, however, since the
		//		output rate is allmost equal to the input rate, i'll
		//		assume the buffer is fast enough
		peripherals[PERIPHERAL_SECONDARY_DATA] = tmp;
		// add \n to \r
		if (tmp == '\r') peripherals[PERIPHERAL_SECONDARY_DATA] = '\n';
		printf("Communication channel 2 Rx: %02X\r\n", tmp);
		if (tmp == 0x1B) quit = 1;
	}
#endif // PERIPHERAL_SECONDARY_DATA
#endif // PERIPHERAL_SECONDARY_STATUS
}

void int_com2_out(void) {
	/* don't do anything on this interrupt */
}

void int_buttons(void) {
	int i, tmp;
	/* one or more of the buttons are being pressed/released. Remember that
		* there is currently NO hardware debouncer available!
		*/
	tmp = peripherals[PERIPHERAL_BUTTONS] & 0x0F;
	printf("Button state: ");
	for (i = 3; i >= 0; i--) {
		printf("%s", tmp & (1<<i) ? "1" : "0");
	}
	printf("\r\n");
}

void int_switches(void) {
	int i, tmp;
	/* one or more of the switches are being changed. Remember that
		* there is currently NO hardware debouncer available!
		*/
	tmp = peripherals[PERIPHERAL_SWITCHES] & 0xFF;
	printf("Switch state: ");
	for (i = 7; i >= 0; i--) {
		printf("%s", tmp & (1<<i) ? "1" : "0");
	}
	printf("\r\n");
}

