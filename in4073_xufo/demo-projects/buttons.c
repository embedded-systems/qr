/*------------------------------------------------------------------
 *  buttons.c -- demo button ISR (and bouncing)
 *------------------------------------------------------------------
 */

#include <stdio.h>
#include "x32.h"

/* define some peripheral short hands
 */
#define X32_display	peripherals[PERIPHERAL_DISPLAY]
#define X32_leds	peripherals[PERIPHERAL_LEDS]
#define X32_buttons	peripherals[PERIPHERAL_BUTTONS]
#define X32_clock	peripherals[PERIPHERAL_MS_CLOCK]

int	demo_done;
int	count;

/*------------------------------------------------------------------
 * isr_buttons -- buttons interrupt service routine
 *------------------------------------------------------------------
 */
void isr_buttons(void)
{
	printf("isr\r\n");
	switch (X32_buttons) { 
		case 0x01: 
			count++;
			X32_display = count;
			break;
		case 0x08:
			count = 0;
			X32_display = count;
			break;
		case 0x09: 
			demo_done = 1;
			break;
	}
}

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
 * main -- do the demo
 *------------------------------------------------------------------
 */
int main() 
{
	/* set ISR vector, priority and enable IRQ
	 */
        SET_INTERRUPT_VECTOR(INTERRUPT_BUTTONS, &isr_buttons);
        SET_INTERRUPT_PRIORITY(INTERRUPT_BUTTONS, 10);
        ENABLE_INTERRUPT(INTERRUPT_BUTTONS);

	/* start the fireworks
	 */
        ENABLE_INTERRUPT(INTERRUPT_GLOBAL); 

	demo_done = 0;
	count = 0;
	while (! demo_done) {
		printf("main: %d\r\n",count);
		delay(300);
	}
	X32_display = 0x0000;

        DISABLE_INTERRUPT(INTERRUPT_GLOBAL);

	return 0;
}



