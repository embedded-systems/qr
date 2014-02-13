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
 *			0x05: display (16 bit, output)
 *			0x06: switches (8 bit, input)
 *			0x07: leds (8 bit, output)
 *			0x08: buttons (4 bit, input)
 * 			0x09: processor state register (8 bit input)
 *			0x20: interrupt enable register (17 bit, input/output)
 *			0x21: software interrupt device (0 bit, output)
 *			0x26: timer 1 (period register) (32 bit, input/output)
 *			0x27: timer 2 (period register) (32 bit, input/output)
 *			
 *			0x30: maxon line 1 status (1 bit, input)
 *			0x31: maxon line 2 status (1 bit, input)
 *			0x32: us clock (32 bit, input)
 *			0x33: maxon decoded status (32 bit, input)
 *			0x40: dpc period
 *			0x41: dpc width
 *			
 *			0x50: xufocomm count
 *			0x51: xufocomm timestamp
 *			0x52: xufocomm sensor 0 // x
 *			0x53: xufocomm sensor 1 // y
 *			0x54: xufocomm sensor 2 // z
 *			0x55: xufocomm sensor 3 // xr
 *			0x56: xufocomm sensor 4 // yr
 *			0x57: xufocomm sensor 5 // compass
 *			0x58: xufocomm sensor 6 // pressure / compass deel 2?
 *			0x59: xufocomm actuator // was: 0x47
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
 *			0x0D: maxon error
 *			0x0E: maxon line 1
 *			0x0F: maxon line 2
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
#include "nexys_pins.h"

#define TARGET xs3s1200e
#define NEXYS
#define CLOCKSPEED 50000000
#define CORE_SIZE_LONG 32
#define CORE_SIZE_INT 32
#define CORE_SIZE_SHORT 16
#define CORE_SIZE_POINTER 32
#define ROM_LOCATION 0x000C0000
#define ROM_ADDRESS_BITS 16

#define INITIAL_RESET buttons(3) = '1'
#define NORMAL_RESET buttons = "1111"

#define PERIPHERAL_ADDRESS_BITS				8
#define PERIPHERAL_DATA_BITS 					32
#define PERIPHERAL_ID									0x4073

#define INSTRUCTION_COUNTER_INDEX 0x03
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

ADD_1TO1_INPUT_DEVICE(switches, 0x06, 0x08, 8, PIN_SWITCHES)
ADD_1TO1_INPUT_DEVICE(buttons, 0x08, 0x07, 4, PIN_BUTTONS)
ADD_1TO1_OUTPUT_DEVICE(leds, 0x07, 8, PIN_LEDS)
ADD_RS232_DEVICE(primary, 0x01, 0x02, 8, 8, 115200, PIN_PRIMARY_TX, PIN_PRIMARY_RX, 0x09, 0x0A)
ADD_4x7SEGDISP_DEVICE(display, 0x05, PIN_DISPLAY_DATA, PIN_DISPLAY_CONTROL)
ADD_CLOCK_DEVICE(ms_clock, 0x04, 50000)
ADD_TIMER_DEVICE(timer1, 0x26, 0x05)
ADD_TIMER_DEVICE(timer2, 0x27, 0x06)
ADD_SOFTINT_DEVICE(softint1, 0x21, 0x04)

// in2305 specific devices
// us counter
ADD_CLOCK_DEVICE(us_clock, 0x32, 50)

// dpc
ADD_DPC_DEVICE(dpc1, 0x40, 0x41, PIN_PMOD_A4)

// 1-on-1 
ADD_1TO1_OUTPUT_DEVICE(custom, 0x34, 1, PIN_PMOD_A3)

// maxon decoder
ADD_MAXON_DEVICE(engine, 0x33, 0x30, 0x31, 0x0D, 0x0E, 0x0F, PIN_PMOD_A2, PIN_PMOD_A1)

// xufocomm
ADD_XUFOCOMM_DEVICE(xufo, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x0C, PIN_PMOD_B1, PIN_PMOD_B2)
