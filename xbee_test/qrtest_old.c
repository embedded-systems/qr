/*------------------------------------------------------------------
 *  qrtest.c -- test QR engines and sensors
 *
 *  reads ae[0-3] from stdin
 *  (q,w,e,r increment ae[0-3], a,s,d,f decrement)
 *
 *  prints ae[0-3],sax,say,saz,sp,sq,sr,delta_t on stdout
 *  where delta_t is the qr-isr exec time
 *
 *  Arjan J.C. van Gemund
 *  Embedded Software Lab
 *
 *  Version Jan 1, 2010
 *------------------------------------------------------------------
 */

#include <stdio.h>
#include <x32.h>
#include "assert.h"

/* define some peripheral short hands
 */
#define X32_leds		peripherals[PERIPHERAL_LEDS]
#define X32_display		peripherals[PERIPHERAL_DISPLAY]
#define X32_ms_clock		peripherals[PERIPHERAL_MS_CLOCK]
#define X32_us_clock		peripherals[PERIPHERAL_US_CLOCK]
#define X32_rs232_data		peripherals[PERIPHERAL_PRIMARY_DATA]
#define X32_rs232_stat		peripherals[PERIPHERAL_PRIMARY_STATUS]
#define X32_rs232_char		(X32_rs232_stat & 0x02)
#define X32_QR_a 		peripherals[PERIPHERAL_XUFO_A]
#define X32_QR_s0 		peripherals[PERIPHERAL_XUFO_S0]
#define X32_QR_s1 		peripherals[PERIPHERAL_XUFO_S1]
#define X32_QR_s2 		peripherals[PERIPHERAL_XUFO_S2]
#define X32_QR_s3 		peripherals[PERIPHERAL_XUFO_S3]
#define X32_QR_s4 		peripherals[PERIPHERAL_XUFO_S4]
#define X32_QR_s5 		peripherals[PERIPHERAL_XUFO_S5]
#define X32_QR_s6               peripherals[PERIPHERAL_XUFO_S6]
#define X32_QR_timestamp 	peripherals[PERIPHERAL_XUFO_TIMESTAMP]

// RX FIFO
#define FIFOSIZE 16
char	fifo[FIFOSIZE]; 
int	iptr, optr;

// Globals
char	c;
int	demo_done;
int	ae[4];
int	s0, s1, s2, s3, s4, s5,s6, timestamp;
int	isr_qr_counter;
int	isr_qr_time;

int gyro_acum;
int gyro_bias;

void	toggle_led(int);
void	delay_ms(int);
void	delay_us(int);


/*------------------------------------------------------------------
 * isr_qr_link -- QR link rx interrupt handler
 *------------------------------------------------------------------
 */
void isr_qr_link(void)
{
	int	ae_index;

	/* record time
	 */
	isr_qr_time = X32_us_clock;

	/* get sensor and timestamp values
	 */
	s0 = X32_QR_s0; s1 = X32_QR_s1; s2 = X32_QR_s2; 
	s3 = X32_QR_s3; s4 = X32_QR_s4; s5 = X32_QR_s5;
	s6 = X32_QR_s6;

	timestamp = X32_QR_timestamp;

	gyro_acum += s5 - gyro_bias;

	/* monitor presence of interrupts 
	 */
	isr_qr_counter++;
	if (isr_qr_counter % 500 == 0) {
		toggle_led(0);
	}
 
	/* send actuator values
	 * (need to supply a continous stream, otherwise
	 * QR will go to safe mode, so just send every ms)
	 */
	for (ae_index = 0; ae_index < 4; ae_index++) {
		X32_QR_a = 0x80 + ae_index;
		delay_us(0);
		X32_QR_a = ((ae[ae_index]) >> 7) & 0x7f;
		delay_us(0);
		X32_QR_a = ae[ae_index] & 0x7f;
		delay_us(0);
	}

	/* record isr execution time (ignore overflow)
	 */
	isr_qr_time = X32_us_clock - isr_qr_time;
}

/*------------------------------------------------------------------
 * isr_rs232_rx -- rs232 rx interrupt handler
 *------------------------------------------------------------------
 */
void isr_rs232_rx(void)
{
	int	c;

	/* signal interrupt
	 */
	toggle_led(1);

	/* may have received > 1 char before IRQ is serviced so loop
	 */
	while (X32_rs232_char) {
		fifo[iptr++] = X32_rs232_data;
		if (iptr > FIFOSIZE)
			iptr = 0;
	}

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
 * delay_ms -- busy-wait for ms milliseconds
 *------------------------------------------------------------------
 */
void delay_ms(int ms) 
{
	int time = X32_ms_clock;
	while(X32_ms_clock - time < ms)
		;
}

/*------------------------------------------------------------------
 * delay_us -- busy-wait for us milliseconds
 *------------------------------------------------------------------
 */
void delay_us(int us) 
{
	int time = X32_us_clock;
	while(X32_us_clock - time < us)
		;
}

/*------------------------------------------------------------------
 * toggle_led -- toggle led # i
 *------------------------------------------------------------------
 */
void toggle_led(int i) 
{
	X32_leds = (X32_leds ^ (1 << i));
}

/*------------------------------------------------------------------
 * process_key -- process command keys
 *------------------------------------------------------------------
 */
void process_key(char c) 
{
	switch (c) {
		case 'q':
			ae[0] += 10;
			break;
		case 'a':
			ae[0] -= 10;
			if (ae[0] < 0) ae[0] = 0;
			break;
		case 'w':
			ae[1] += 10;
			break;
		case 's':
			ae[1] -= 10;
			if (ae[1] < 0) ae[1] = 0;
			break;
		case 'e':
			ae[2] += 10;
			break;
		case 'd':
			ae[2] -= 10;
			if (ae[2] < 0) ae[2] = 0;
			break;
		case 'r':
			ae[3] += 10;
			break;
		case 'f':
			ae[3] -= 10;
			if (ae[3] < 0) ae[3] = 0;
			break;
	case 'm':
	  gyro_acum = 0;
	  gyro_bias = s5;
	  break;
		default:
			demo_done = 1;
	}
}

/*------------------------------------------------------------------
 * print_state -- print all sensors and actuators
 *------------------------------------------------------------------
 */
void print_state(void) 
{
	printf("%3d %3d %3d %3d | ",ae[0],ae[1],ae[2],ae[3]);
	printf("%3d %3d %3d %3d %3d %3d | %3d (%3d) - %3d\r\n",
	       s0,s1,s2,s3,s4,s5,s6,isr_qr_time, timestamp);
}

/*------------------------------------------------------------------
 * main -- do the test
 *------------------------------------------------------------------
 */
int main() 
{
	/* prepare QR rx interrupt handler
	 */
        SET_INTERRUPT_VECTOR(INTERRUPT_XUFO, &isr_qr_link);
        SET_INTERRUPT_PRIORITY(INTERRUPT_XUFO, 21);
	isr_qr_counter = isr_qr_time = 0;
	ae[0] = ae[1] = ae[2] = ae[3] = 0;
        ENABLE_INTERRUPT(INTERRUPT_XUFO);
 
	/* prepare rs232 rx interrupt and getchar handler
	 */
        SET_INTERRUPT_VECTOR(INTERRUPT_PRIMARY_RX, &isr_rs232_rx);
        SET_INTERRUPT_PRIORITY(INTERRUPT_PRIMARY_RX, 20);
	while (X32_rs232_char) c = X32_rs232_data; // empty buffer
	iptr = optr = 0;
        ENABLE_INTERRUPT(INTERRUPT_PRIMARY_RX);

	/* initialize some other stuff
	 */
	X32_leds = X32_display = 0;
	demo_done = 0;

	/* start the test loop
	 */
        ENABLE_INTERRUPT(INTERRUPT_GLOBAL); 

	while (! demo_done) {
		c = getchar();
		if (c != -1) {
			process_key(c);
		}
		print_state();
		delay_ms(20);
	}
	printf("Exit\r\n");

        DISABLE_INTERRUPT(INTERRUPT_GLOBAL);

	return 0;
}

