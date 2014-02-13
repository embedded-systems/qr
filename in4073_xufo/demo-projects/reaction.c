/*------------------------------------------------------------------
 *  reaction.c -- reaction timer (cf. VHDL project)
 *------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include "x32.h"
#include "assert.h"

/* define some peripheral short hands
 */
#define X32_display	peripherals[PERIPHERAL_DISPLAY]
#define X32_leds	peripherals[PERIPHERAL_LEDS]
#define X32_buttons	peripherals[PERIPHERAL_BUTTONS]
#define X32_clock	peripherals[PERIPHERAL_MS_CLOCK]
#define X32_rs232_data  peripherals[PERIPHERAL_PRIMARY_DATA]
#define X32_rs232_stat  peripherals[PERIPHERAL_PRIMARY_STATUS]
#define X32_rs232_char  (X32_rs232_stat & 0x02)


/*------------------------------------------------------------------
 * isr_rs232_rx -- rs232 rx interrupt handler
 *------------------------------------------------------------------
 */
void isr_rs232_rx(void)
{
	exit(0);
}

/*------------------------------------------------------------------
 * delay -- busy-wait for ms milliseconds
 *------------------------------------------------------------------
 */
void	delay(int ms) 
{
	int time = X32_clock;
	while(X32_clock - time < ms)
		;
}

/*------------------------------------------------------------------
 * demo -- do the demo
 *------------------------------------------------------------------
 */
void 	demo() 
{
	int	state;
	int	time;

	printf("hit key to exit ..\r\n");

	state = 0;
	X32_leds = 0x00;
	while (! X32_rs232_char) {
		switch (state) {
			case 0: // standby to react ..
				X32_display = 0;
				delay(5000);
				time = X32_clock;
				X32_leds = 0xff;
				state = 1;
				break;
			case 1: // start counting ..
				X32_display = (X32_clock - time);
				if (X32_buttons) {
					delay(20);
					while (X32_buttons) ;
					delay(20);
					X32_leds = 0x00;
					state = 2;
				}
				break;
			case 2: // pressed, wait for redo
				if (X32_buttons)
					state = 0;
				break;
		}
	}
}

/*------------------------------------------------------------------
 * main -- setup the demo
 *------------------------------------------------------------------
 */
int main() 
{
    char unused;
	/* prepare rs232 rx interrupt
	 */
        SET_INTERRUPT_VECTOR(INTERRUPT_PRIMARY_RX, &isr_rs232_rx);
        SET_INTERRUPT_PRIORITY(INTERRUPT_PRIMARY_RX, 20);
	while (X32_rs232_char) unused = X32_rs232_data; // empty buffer
        ENABLE_INTERRUPT(INTERRUPT_PRIMARY_RX);

	/* start the fireworks
	 */
        ENABLE_INTERRUPT(INTERRUPT_GLOBAL); 

	demo();

        DISABLE_INTERRUPT(INTERRUPT_GLOBAL);
	return 0;
}

