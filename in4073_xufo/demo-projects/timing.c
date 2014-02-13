/*------------------------------------------------------------------
 *  timing.c -- demo IRQ response time
 *------------------------------------------------------------------
 */

#include <stdio.h>
#include "x32.h"

/* define some peripheral short hands
 */
#define X32_display	peripherals[PERIPHERAL_DISPLAY]
#define X32_buttons	peripherals[PERIPHERAL_BUTTONS]
#define X32_clock       peripherals[PERIPHERAL_MS_CLOCK]
#define X32_timer_per   peripherals[PERIPHERAL_TIMER1_PERIOD]


int	demo_done;
int	last_time;
int	first_time;
int	clock_start;

/*------------------------------------------------------------------
 * delay -- busy-wait for ms milliseconds
 *------------------------------------------------------------------
 */
void delay(int ms)
{
        int time = X32_clock;
        while(X32_clock - time < ms)
                ;
}

/*------------------------------------------------------------------
 * isr_buttons -- buttons interrupt handler
 *------------------------------------------------------------------
 */
void isr_buttons(void)
{
	// printf("%d: isr_button\r\n",X32_clock);
	// waste some serious time
	delay(100);
	if (X32_buttons == 0x09)
		demo_done = 1;
}

/*------------------------------------------------------------------
 * isr_timer -- timer interrupt handler
 *------------------------------------------------------------------
 */
void isr_timer(void)
{
	int	clock;

	clock = X32_clock - clock_start;
	printf("%d: isr_timer (%d)\r\n",clock,last_time);

	if (clock - last_time > 1000)
		printf("*** interrupt latency: %d\r\n",
			clock - last_time - 1000);
	last_time += 1000;
}

/*------------------------------------------------------------------
 * main -- do the demo
 *------------------------------------------------------------------
 */
int main() 
{
	int	copy1, copy2;

	/* prepare button interrupt
	 */
        SET_INTERRUPT_VECTOR(INTERRUPT_BUTTONS, &isr_buttons);
        SET_INTERRUPT_PRIORITY(INTERRUPT_BUTTONS, 10);
        ENABLE_INTERRUPT(INTERRUPT_BUTTONS);

	/* prepare timer interrupt every 1 s
	 */
        X32_timer_per = 1000 * CLOCKS_PER_MS;
        SET_INTERRUPT_VECTOR(INTERRUPT_TIMER1, &isr_timer);
        SET_INTERRUPT_PRIORITY(INTERRUPT_TIMER1, 9);
        ENABLE_INTERRUPT(INTERRUPT_TIMER1);

	/* start the fireworks
	 */
        ENABLE_INTERRUPT(INTERRUPT_GLOBAL); 

	clock_start = X32_clock;
	last_time = 0;
	demo_done = 0;
	while (! demo_done) {
        	DISABLE_INTERRUPT(INTERRUPT_GLOBAL);
		// waste some serious time
		delay(500);
        	ENABLE_INTERRUPT(INTERRUPT_GLOBAL);
		delay(500);
		printf("%d: main\r\n",X32_clock - clock_start);
	}

        DISABLE_INTERRUPT(INTERRUPT_GLOBAL);

	return 0;
}



