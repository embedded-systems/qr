/*------------------------------------------------------------------
 *  console.c -- demo console I/O and fifo buffering
 *------------------------------------------------------------------
 */

#include <stdio.h>
#include "x32.h"
#include "assert.h"

/* define some peripheral short hands
 */
#define X32_display	peripherals[PERIPHERAL_DISPLAY]
#define X32_clock	peripherals[PERIPHERAL_MS_CLOCK]
#define X32_rs232_data	peripherals[PERIPHERAL_PRIMARY_DATA]
#define X32_rs232_stat	peripherals[PERIPHERAL_PRIMARY_STATUS]
#define X32_rs232_char	(X32_rs232_stat & 0x02)

int	demo_done;
int	isr_tx_cntr;

#define FIFOSIZE 16
char	fifo[FIFOSIZE]; 
int	iptr, optr;

/*------------------------------------------------------------------
 * isr_rs232_rx -- rs232 rx interrupt handler
 *------------------------------------------------------------------
 */
void isr_rs232_rx(void)
{
	int	c;

	// may have received > 1 char before IRQ is serviced so loop
	while (X32_rs232_char) {
		fifo[iptr++] = X32_rs232_data;
		if (iptr > FIFOSIZE)
			iptr = 0;
	}

}

/*------------------------------------------------------------------
 * isr_rs232_tx -- rs232 tx interrupt handler
 *------------------------------------------------------------------
 */
void isr_rs232_tx(void)
{
	X32_display = ++isr_tx_cntr;
}

/*------------------------------------------------------------------
 * getchar -- read char from rx fifo, return -1 if no char available
 *------------------------------------------------------------------
 */
int 	getchar(void)
{
	int	c;

	if (optr == iptr)
		return -1;
	c = fifo[optr++];
	if (optr > FIFOSIZE)
		optr = 0;
	return c;
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
	char	c;

	/* prepare rs232 rx interrupt
	 */
        SET_INTERRUPT_VECTOR(INTERRUPT_PRIMARY_RX, &isr_rs232_rx);
        SET_INTERRUPT_PRIORITY(INTERRUPT_PRIMARY_RX, 20);
	while (X32_rs232_char) c = X32_rs232_data; // empty buffer
        ENABLE_INTERRUPT(INTERRUPT_PRIMARY_RX);

	/* prepare rs232 tx interrupt
	 */
	isr_tx_cntr = 0;
        SET_INTERRUPT_VECTOR(INTERRUPT_PRIMARY_TX, &isr_rs232_tx);
        SET_INTERRUPT_PRIORITY(INTERRUPT_PRIMARY_TX, 15);
        ENABLE_INTERRUPT(INTERRUPT_PRIMARY_TX);

	/* start the fireworks
	 */
        ENABLE_INTERRUPT(INTERRUPT_GLOBAL); 

	demo_done = 0;
	iptr = 0;
	optr = 0;
	while (! demo_done) {
		c = getchar();
		if (c != -1)
			printf("%c",c);
		delay(100);
		demo_done = (c == 'q');
	}
	printf("\r\n");

        DISABLE_INTERRUPT(INTERRUPT_GLOBAL);

	return 0;
}

