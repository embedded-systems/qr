#include <assert.h>
#include <limits.h>

#ifndef IX86
#include <x32.h>

int overflow_val;
int overflow_int;
int div0_val;
int div0_int;

static int i(int i) { return i; }
static unsigned int u(unsigned int u) { return u; }
static void isr_overflow() { overflow_int = 1; }
static void isr_div0() { div0_int = 1; }

static int div_signed(int a, int b) {
	int c;
	// reset
	div0_int = 0;	c = STATE_DIVISION_BY_ZERO;
	c = a/b;
	return (div0_int << 1) | (STATE_DIVISION_BY_ZERO?1:0);
}

static int mod_signed(unsigned a, unsigned b) {
	unsigned c;
	// reset
	div0_int = 0;	c = STATE_DIVISION_BY_ZERO;
	c = a%b;
	return (div0_int << 1) | (STATE_DIVISION_BY_ZERO?1:0);
}

static int div_unsigned(unsigned a, unsigned b) {
	unsigned c;
	// reset
	div0_int = 0;	c = STATE_DIVISION_BY_ZERO;
	c = a/b;
	return (div0_int << 1) | (STATE_DIVISION_BY_ZERO?1:0);
}

static int mod_unsigned(int a, int b) {
	int c;
	// reset
	div0_int = 0;	c = STATE_DIVISION_BY_ZERO;
	c = a%b;
	return (div0_int << 1) | (STATE_DIVISION_BY_ZERO?1:0);
}


static int add_signed32(int a, int b) {
	int c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a+b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int sub_signed32(int a, int b) {
	int c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a-b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int mul_signed32(int a, int b) {
	int c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a*b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int add_unsigned32(unsigned a, unsigned b) {
	unsigned c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a+b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int sub_unsigned32(unsigned a, unsigned b) {
	unsigned c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a-b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int mul_unsigned32(unsigned a, unsigned b) {
	unsigned c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a*b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int add_signed16(short a, short b) {
	short c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a+b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int sub_signed16(short a, short b) {
	short c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a-b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int mul_signed16(short a, short b) {
	short c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a*b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int add_unsigned16(unsigned short a, unsigned short b) {
	unsigned short c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a+b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int sub_unsigned16(unsigned short a, unsigned short b) {
	unsigned short c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a-b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int mul_unsigned16(unsigned short a, unsigned short b) {
	unsigned short c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a*b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int add_signed8(char a, char b) {
	signed char c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a+b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int sub_signed8(char a, char b) {
	signed char c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a-b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int mul_signed8(char a, char b) {
	signed char c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a*b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int add_unsigned8(unsigned char a, unsigned char b) {
	unsigned char c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a+b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int sub_unsigned8(unsigned char a, unsigned char b) {
	unsigned char c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a-b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

static int mul_unsigned8(unsigned char a, unsigned char b) {
	unsigned char c;
	// reset
	overflow_int = 0;	c = STATE_OVERFLOW;
	c = a*b;
	return (overflow_int << 1) | (STATE_OVERFLOW?1:0);
}

void test_alu_errors() {
	overflow_val = 0;
	div0_val = 0;
	#ifdef INTERRUPT_OVERFLOW
		INTERRUPT_VECTOR(INTERRUPT_OVERFLOW) = &isr_overflow;
		INTERRUPT_PRIORITY(INTERRUPT_OVERFLOW) = 10;
		ENABLE_INTERRUPT(INTERRUPT_OVERFLOW);
		overflow_val |= 2;
	#else
		#warning No overflow interrupt detected, not testing
	#endif
	#ifdef INTERRUPT_DIVISION_BY_ZERO
		INTERRUPT_VECTOR(INTERRUPT_DIVISION_BY_ZERO) = &isr_div0;
		INTERRUPT_PRIORITY(INTERRUPT_DIVISION_BY_ZERO) = 10;
		ENABLE_INTERRUPT(INTERRUPT_DIVISION_BY_ZERO);
		div0_val |= 2;
	#else
		#warning No division by zero interrupt detected, not testing
	#endif
	#ifdef PERIPHERAL_PROCSTATE
		div0_val |= 1;
		overflow_val |= 1;
	#else
		#warning No processor state register, not testing
	#endif

	//TODO
	//printf("-%d = %d\n", i(LONG_MAX), -i(LONG_MAX));
	//printf("-%d = %d\n", i(LONG_MIN), -i(LONG_MIN));

	// add signed 32 ok
	assert(add_signed32(1,0)==0);
	assert(add_signed32(0,1)==0);
	assert(add_signed32(-1,0)==0);
	assert(add_signed32(0,-1)==0);
	assert(add_signed32(1000,1000)==0);
	assert(add_signed32(1000,-1000)==0);
	assert(add_signed32(-1000,1000)==0);
	assert(add_signed32(-1000,-1000)==0);

	// add signed 32 overflow
	assert(add_signed32(2000000000,2000000000)==overflow_val);
	assert(add_signed32(2000000000,-2000000000)==0);
	assert(add_signed32(-2000000000,2000000000)==0);
	assert(add_signed32(-2000000000,-2000000000)==overflow_val);

	// sub signed 32 ok
	assert(sub_signed32(1,0)==0);
	assert(sub_signed32(0,1)==0);
	assert(sub_signed32(-1,0)==0);
	assert(sub_signed32(0,-1)==0);
	assert(sub_signed32(1000,1000)==0);
	assert(sub_signed32(1000,-1000)==0);
	assert(sub_signed32(-1000,1000)==0);
	assert(sub_signed32(-1000,-1000)==0);

	// sub signed 32 overflow
	assert(sub_signed32(2000000000,2000000000)==0);
	assert(sub_signed32(2000000000,-2000000000)==overflow_val);
	assert(sub_signed32(-2000000000,2000000000)==overflow_val);
	assert(sub_signed32(-2000000000,-2000000000)==0);

	// mul signed 32 ok
	assert(mul_signed32(0,1)==0);
	assert(mul_signed32(1,0)==0);
	assert(mul_signed32(0,-1)==0);
	assert(mul_signed32(-1,0)==0);
	assert(mul_signed32(1,-1)==0);
	assert(mul_signed32(-1,1)==0);
	assert(mul_signed32(1000,1000)==0);
	assert(mul_signed32(1000,-1000)==0);
	assert(mul_signed32(-1000,1000)==0);
	assert(mul_signed32(-1000,-1000)==0);

	// mul signed 32 overflow
	assert(mul_signed32(2000000000,2000000000)==overflow_val);
	assert(mul_signed32(2000000000,-2000000000)==overflow_val);
	assert(mul_signed32(-2000000000,2000000000)==overflow_val);
	assert(mul_signed32(-2000000000,-2000000000)==overflow_val);
	assert(mul_signed32(60000,60000)==overflow_val);
	assert(mul_signed32(60000,-60000)==overflow_val);
	assert(mul_signed32(-60000,60000)==overflow_val);
	assert(mul_signed32(-60000,-60000)==overflow_val);

	// add unsigned 32 ok
	assert(add_unsigned32(2000000000,2000000000)==0);

	// add unsigned 32 overflow
	assert(add_unsigned32(3000000000,3000000000)==overflow_val);

	// sub unsigned 32 ok
	assert(sub_unsigned32(2000000000,2000000000)==0);

	// sub unsigned 32 overflow
	assert(sub_unsigned32(10,15)==overflow_val);

	// mul unsigned 32 ok
	assert(mul_unsigned32(60000,60000)==0);

	// mul unsigned 32 overflow
	assert(mul_unsigned32(2000000000,2000000000)==overflow_val);
	assert(mul_unsigned32(70000,70000)==overflow_val);

	// add signed 16 ok
	assert(add_signed16(1000,1000)==0);
	assert(add_signed16(1000,-1000)==0);
	assert(add_signed16(-1000,1000)==0);
	assert(add_signed16(-1000,-1000)==0);

	// add signed 16 overflow
	assert(add_signed16(30000,30000)==overflow_val);
	assert(add_signed16(30000,-30000)==0);
	assert(add_signed16(-30000,30000)==0);
	assert(add_signed16(-30000,-30000)==overflow_val);

	// sub signed 16 ok
	assert(sub_signed16(1000,1000)==0);
	assert(sub_signed16(1000,-1000)==0);
	assert(sub_signed16(-1000,1000)==0);
	assert(sub_signed16(-1000,-1000)==0);

	// sub signed 16 overflow
	assert(sub_signed16(30000,30000)==0);
	assert(sub_signed16(30000,-30000)==overflow_val);
	assert(sub_signed16(-30000,30000)==overflow_val);
	assert(sub_signed16(-30000,-30000)==0);

	// mul signed 16 ok
	assert(mul_signed16(100,100)==0);
	assert(mul_signed16(100,-100)==0);
	assert(mul_signed16(-100,100)==0);
	assert(mul_signed16(-100,-100)==0);

	// mul signed 16 overflow
	assert(mul_signed16(30000,30000)==overflow_val);
	assert(mul_signed16(30000,-30000)==overflow_val);
	assert(mul_signed16(-30000,30000)==overflow_val);
	assert(mul_signed16(-30000,-30000)==overflow_val);
	assert(mul_signed16(200,200)==overflow_val);
	assert(mul_signed16(200,-200)==overflow_val);
	assert(mul_signed16(-200,200)==overflow_val);
	assert(mul_signed16(-200,-200)==overflow_val);

	// add unsigned 16 ok
	assert(add_unsigned16(30000,30000)==0);

	// add unsigned 16 overflow
	assert(add_unsigned16(40000,40000)==overflow_val);

	// sub unsigned 16 ok
	assert(sub_unsigned16(40000,40000)==0);

	// sub unsigned 16 overflow
	assert(sub_unsigned16(10,15)==overflow_val);

	// mul unsigned 16 ok
	assert(mul_unsigned16(200,200)==0);

	// mul unsigned 16 overflow
	assert(mul_unsigned16(30000,30000)==overflow_val);
	assert(mul_unsigned16(600,600)==overflow_val);

	// add signed 8 ok
	assert(add_signed8(10,10)==0);
	assert(add_signed8(10,-10)==0);
	assert(add_signed8(-10,10)==0);
	assert(add_signed8(-10,-10)==0);

	// add signed 8 overflow
	assert(add_signed8(100,100)==overflow_val);
	assert(add_signed8(100,-100)==0);
	assert(add_signed8(-100,100)==0);
	assert(add_signed8(-100,-100)==overflow_val);

	// sub signed 8 ok
	assert(sub_signed8(10,10)==0);
	assert(sub_signed8(10,-10)==0);
	assert(sub_signed8(-10,10)==0);
	assert(sub_signed8(-10,-10)==0);

	// sub signed 8 overflow
	assert(sub_signed8(100,100)==0);
	assert(sub_signed8(100,-100)==overflow_val);
	assert(sub_signed8(-100,100)==overflow_val);
	assert(sub_signed8(-100,-100)==0);

	// mul signed 8 ok
	assert(mul_signed8(10,10)==0);
	assert(mul_signed8(10,-10)==0);
	assert(mul_signed8(-10,10)==0);
	assert(mul_signed8(-10,-10)==0);

	// mul signed 8 overflow
	assert(mul_signed8(100,100)==overflow_val);
	assert(mul_signed8(100,-100)==overflow_val);
	assert(mul_signed8(-100,100)==overflow_val);
	assert(mul_signed8(-100,-100)==overflow_val);
	assert(mul_signed8(10,20)==overflow_val);
	assert(mul_signed8(10,-20)==overflow_val);
	assert(mul_signed8(-10,20)==overflow_val);
	assert(mul_signed8(-10,-20)==overflow_val);

	// add unsigned 8 ok
	assert(add_unsigned8(100,100)==0);

	// add unsigned 8 overflow
	assert(add_unsigned8(200,200)==overflow_val);

	// sub unsigned 8 ok
	assert(sub_unsigned8(200,200)==0);

	// sub unsigned 8 overflow
	assert(sub_unsigned8(10,15)==overflow_val);

	// mul unsigned 8 ok
	assert(mul_unsigned8(10,20)==0);

	// mul unsigned 8 overflow
	assert(mul_unsigned8(100,100)==overflow_val);
	assert(mul_unsigned8(20,20)==overflow_val);

	// div signed
	assert(div_signed(10, 1)==0);
	assert(div_signed(10, 0)==div0_val);
	assert(div_signed(-10, 1)==0);
	assert(div_signed(-10, 0)==div0_val);
	assert(div_signed(10, -1)==0);
	assert(div_signed(10, 0)==div0_val);
	assert(div_signed(-10, -1)==0);
	assert(div_signed(-10, 0)==div0_val);

	// mod signed
	assert(mod_signed(10, 1)==0);
	assert(mod_signed(10, 0)==div0_val);
	assert(mod_signed(-10, 1)==0);
	assert(mod_signed(-10, 0)==div0_val);
	assert(mod_signed(10, -1)==0);
	assert(mod_signed(10, 0)==div0_val);
	assert(mod_signed(-10, -1)==0);
	assert(mod_signed(-10, 0)==div0_val);

	// div unsigned
	assert(div_unsigned(10, 1)==0);
	assert(div_unsigned(10, 0)==div0_val);

	// mod unsigned
	assert(mod_unsigned(10, 1)==0);
	assert(mod_unsigned(10, 0)==div0_val);


	#ifdef INTERRUPT_OVERFLOW
		DISABLE_INTERRUPT(INTERRUPT_OVERFLOW);
	#endif
	#ifdef INTERRUPT_DIVISION_BY_ZERO
		DISABLE_INTERRUPT(INTERRUPT_DIVISION_BY_ZERO);
	#endif

}
#else
void test_alu_errors() {
}
#endif

