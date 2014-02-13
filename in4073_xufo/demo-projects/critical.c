/*------------------------------------------------------------------
 *  critical.c -- demo critical sections
 *------------------------------------------------------------------
 */

#include <stdio.h>
#include "x32.h"

/* define some peripheral short hands
 */
#define X32_display	peripherals[PERIPHERAL_DISPLAY]
#define X32_buttons	peripherals[PERIPHERAL_BUTTONS]

int	demo_done;
int	x, y;

/*------------------------------------------------------------------
 * isr_buttons -- buttons interrupt handler
 *------------------------------------------------------------------
 */
void isr_buttons(void)
{
	printf("isr: %d++\r\n",x);
	x++;
	y++;
	if (X32_buttons == 0x09)
		demo_done = 1;
}

/*------------------------------------------------------------------
 * main -- do the demo
 *------------------------------------------------------------------
 */
int main() 
{
	int	copy1, copy2;

	/* set ISR vector, priority and enable IRQ
	 */
        SET_INTERRUPT_VECTOR(INTERRUPT_BUTTONS, &isr_buttons);
        SET_INTERRUPT_PRIORITY(INTERRUPT_BUTTONS, 10);
        ENABLE_INTERRUPT(INTERRUPT_BUTTONS);

	/* start the fireworks
	 */
        ENABLE_INTERRUPT(INTERRUPT_GLOBAL); 

	demo_done = 0;
	x = y = 0;
	while (! demo_done) {
        	// DISABLE_INTERRUPT(INTERRUPT_BUTTONS);
		copy1 = x;
		copy2 = y;
        	// ENABLE_INTERRUPT(INTERRUPT_BUTTONS);
		X32_display = ((copy1 & 0xff) << 8) | (copy2 & 0xff);
		if (copy1 != copy2)
			printf("alarm: %d != %d\r\n",copy1,copy2);
	}

        DISABLE_INTERRUPT(INTERRUPT_GLOBAL);

	return 0;
}



