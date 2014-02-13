/*
 * IN2305+IN4073 T-REX configuration file
 */

/*
 *		Device list:
 *			0x00: bus id (16 bit, input)
 *			0x01: primary rs232 data port (8 bit, input/output)
 *			0x02: primary rs232 control register (2 bit, input)
 *			0x03: instruction counter (32 bit, input)
 *			0x04: millisecond clock (32 bit, input)
 *			0x06: switches (8 bit, input)
 *			0x07: leds (2 bit, output)
 *			0x08: buttons (1 bit, input)
 * 			0x09: processor state register (8 bit input)
 *                      0x0A: cache miss counter (32 bit output)
 *			0x11: wireless rs232 data port (8 bit, input/output)
 *			0x12: wireless rs232 control register (2 bit, input)
 *			0x20: interrupt enable register (17 bit, input/output)
 *			0x21: software interrupt device (0 bit, output)
 *			0x26: timer 1 (period register) (32 bit, input/output)
 *			0x27: timer 2 (period register) (32 bit, input/output)
 *			0x32: us clock (32 bit, input)
 *			
 *			0x50: xufocomm count
 *			0x51: xufocomm timestamp
 *			0x52: xufocomm sensor 0 // x
 *			0x53: xufocomm sensor 1 // y
 *			0x54: xufocomm sensor 2 // z
 *			0x55: xufocomm sensor 3 // xr
 *			0x56: xufocomm sensor 4 // yr
 *			0x57: xufocomm sensor 5 // zr
 *			0x58: xufocomm sensor 6 // battery level
 *			0x59: xufocomm A0       //Engine 0 
 *			0x59: xufocomm A1       //Engine 0 
 *			0x59: xufocomm A2       //Engine 0 
 *			0x59: xufocomm A3       //Engine 0 
 *			
 *		Interrupt list:
 *			0x00: trap instruction
 *			0x01: division by zero
 *			0x02: overflow
 *			0x03: out of memory
 *			0x04: software interrupt
 *			0x05: timer 1
 *			0x06: timer 2
 *			0x07: buttons
 *			0x08: switches
 *			0x09: primary rs232 rx
 *			0x0A: primary rs232 tx
 *			0x0B: 
 *			0x0C: xufo_comm
 *			0x0D: 
 *			0x0E: wireless rx
 *			0x0F: wireless tx
 */

#include "peripherals/1to1.vh"
#include "peripherals/rs232.vh"
#include "peripherals/4x7seg.vh"
#include "peripherals/clock.vh"
#include "peripherals/timer.vh"
#include "peripherals/softint.vh"
#include "peripherals/dpc.vh"
#include "peripherals/maxon.vh"
#include "peripherals/xufo_comm.vh"
#include "te300.h"

#define TARGET xs3s1600e
#define TE300
#define CLOCKSPEED 50000000
#define CORE_SIZE_LONG 32
#define CORE_SIZE_INT 32
#define CORE_SIZE_SHORT 16
#define CORE_SIZE_POINTER 32
#define ROM_LOCATION 0x000C0000
#define ROM_SIZE 0x00006000
#define ROM_ADDRESS_BITS 16

#define INITIAL_RESET
#define NORMAL_RESET
//#define INITIAL_RESET buttons(3) = '1'
//#define NORMAL_RESET buttons = "1111"



#define PERIPHERAL_ADDRESS_BITS				8
#define PERIPHERAL_DATA_BITS 				32
#define PERIPHERAL_ID						0x4073

#define INSTRUCTION_COUNTER_INDEX 0x03
#define CACHEMISS_COUNTER_INDEX 0x0A
#define PROCSTATE_REGISTER_INDEX 0x09
#define INTERRUPTS_ENABLE
#define INTERRUPT_COUNT 16
#define INTERRUPT_ENABLE_INDEX 0x20
#define INTERRUPT_CRITICAL 5

#define TRAP_IRQ 0x00
#define OVERFLOW_IRQ 0x02
#define DIV0_IRQ 0x01

#define OOM_IRQ 0x03
#define OOM_UPPER_BOUND 0x7FFFFFFF
#define OOM_LOWER_BOUND 0x000FF000

#define LIBCODE_X32_MS_CLOCK (peripherals[0x04])
#define LIBCODE_X32_US_CLOCK (peripherals[0x32])
#define LIBCODE_X32_STDOUT (peripherals[0x01])
#define LIBCODE_X32_STDOUT_STATUS (peripherals[0x02] & 0x01)
#define LIBCODE_X32_STDIN (peripherals[0x01])
#define LIBCODE_X32_STDIN_STATUS (peripherals[0x02] & 0x02)


ADD_CLOCK_DEVICE(ms_clock, 0x04, 50000)

ADD_1TO1_INPUT_DEVICE(switches, 0x06, 0x08, 2, PIN_SWITCHES)

ADD_1TO1_OUTPUT_DEVICE(leds, 0x07, 8, PIN_LEDS)

ADD_1TO1_INPUT_DEVICE(buttons, 0x08, 0x07, 1, PIN_BUTTON)



ADD_CLOCK_DEVICE(us_clock, 0x32, 50)
ADD_TIMER_DEVICE(timer1, 0x26, 0x05)
//ADD_TIMER_DEVICE(timer2, 0x27, 0x06)
ADD_SOFTINT_DEVICE(softint1, 0x21, 0x04)
// PC comm
ADD_RS232_DEVICE(primary, 0x01, 0x02, 8, 8, 115200, PIN_PRIMARY_TX, PIN_PRIMARY_RX, 0x09, 0x0A)
// Wireless comm
ADD_RS232_DEVICE(wireless, 0x11, 0x12, 8, 8, 9600, PIN_WIRELESS_TX, PIN_WIRELESS_RX, 0x0E, 0x0F)
// QR comm
ADD_XUFOCOMM_DEVICE(xufo, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x0C, PIN_QR_TX, PIN_QR_RX , 0x59,0x5A,0x5B,0x5C)

