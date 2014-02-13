/*------------------------------------------------------------------
 *  leds.c -- copy buttons to leds + display clock
 *            (no isr)
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


/*------------------------------------------------------------------
 * main -- do the demo
 *------------------------------------------------------------------
 */
int main() 
{
	printf("Hello World!\r\n");
	while (1) {
		X32_display = X32_clock;
		X32_leds = X32_buttons;
		if (X32_buttons == 0x09)
			break;
	}
	return 0;
}
