#ifndef INTERPRETER_PERIPHERALS_H
#define INTERPRETER_PERIPHERALS_H

#define INSTRCTIONS_PER_MS		3300

#define OOM_UPPER_BOUND 0x7FFFFFFF
#define OOM_LOWER_BOUND 0x000FF000

#include "interpreter_engine.h"
#include "io.h"
#include "bool.h"

/* initialize peripherals, should be called before
		setting/getting any peripheral values */
int init_peripherals();
/* terminate peripherals, should be called on program
		exit */
void term_peripherals();

/* get a value from a peripheral (index) */
Value* get_peripheral_value(Interpreter*, int);		
/* set a value to a peripheral (index) */
void set_peripheral_value(Interpreter*, int, Value*);
/* check whether an interrupt is pending: returns 1 if so, the two
			int pointers are then filled with the address and the priority
			of the interrupt, the last integer should contain the minimum
 			priority an interrupt should have to be able to trigger */
int interrupt_pending(unsigned*, unsigned*, unsigned, BOOL, BOOL, BOOL, BOOL);

#endif // INTERPRETER_PERIPHERALS_H
