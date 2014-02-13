#include <assert.h>

static int i(int i) { return i; }
static unsigned int u(unsigned int u) { return u; }

void test_math() {
	// signed addition
	assert(i(10)+i(5)==i(15));
	assert(i(-10)+i(5)==i(-5));
	assert(i(10)+i(-5)==i(5));
	assert(i(-10)+i(-5)==i(-15));

	// signed addition with overflow
	assert(i(2000000000)+i(1500000000)==i(-794967296));
	assert(i(-2000000000)+i(1500000000)==i(-500000000));
	assert(i(2000000000)+i(-1500000000)==i(500000000));
	assert(i(-2000000000)+i(-1500000000)==i(794967296));

	// signed subtraction
	assert(i(10)-i(5)==i(5));
	assert(i(-10)-i(5)==i(-15));
	assert(i(10)-i(-5)==i(15));
	assert(i(-10)-i(-5)==i(-5));

	// signed subtraction with overflow
	assert(i(2000000000)-i(1500000000)==i(500000000));
	assert(i(-2000000000)-i(1500000000)==i(794967296));
	assert(i(2000000000)-i(-1500000000)==i(-794967296));
	assert(i(-2000000000)-i(-1500000000)==i(-500000000));

	// signed multiplication
	assert(i(10)*i(5)==i(50));
	assert(i(-10)*i(5)==i(-50));
	assert(i(10)*i(-5)==i(-50));
	assert(i(-10)*i(-5)==i(50));

	// signed multiplication with overflow
	assert(i(100000)*i(80000)==i(-589934592));
	assert(i(-100000)*i(80000)==i(589934592));
	assert(i(100000)*i(-80000)==i(589934592));
	assert(i(-100000)*i(-80000)==i(-589934592));
	assert(i(-1)*i(1)==i(-1));

	// signed division
	assert(i(10)/i(5)==i(2));
	assert(i(-10)/i(5)==i(-2));
	assert(i(10)/i(-5)==i(-2));
	assert(i(-10)/i(-5)==i(2));

	// signed division (non dividable)
	assert(i(13)/i(5)==i(2));
	assert(i(-13)/i(5)==i(-2));
	assert(i(13)/i(-5)==i(-2));
	assert(i(-13)/i(-5)==i(2));
	assert(i(13)/i(20)==i(0));
	assert(i(-13)/i(20)==i(0));
	assert(i(13)/i(-20)==i(0));
	assert(i(-13)/i(-20)==i(0));

	// signed modulo (dividable)
	assert(i(10)%i(5)==i(0));
	assert(i(-10)%i(5)==i(0));
	assert(i(10)%i(-5)==i(0));
	assert(i(-10)%i(-5)==i(0));

	// signed modulo (non dividable)
	assert(i(13)%i(5)==i(3));
	assert(i(-13)%i(5)==i(-3));
	assert(i(13)%i(-5)==i(3));
	assert(i(-13)%i(-5)==i(-3));
	assert(i(13)%i(20)==i(13));
	assert(i(-13)%i(20)==i(-13));
	assert(i(13)%i(-20)==i(13));
	assert(i(-13)%i(-20)==i(-13));

	// signed left shift
	assert(i(13)<<i(5)==i(416));
	assert(i(-13)<<i(5)==i(-416));
	assert(i(13)<<i(30)==i(1073741824));
	assert(i(-13)<<i(30)==i(-1073741824));
	#ifndef IX86
		// produces different results at x32 and ix86!, use -DSIMPLE_LSH when 
		//  compiling on gcc
		assert(i(13)<<i(32)==i(0));
		assert(i(-13)<<i(32)==i(0));
	#endif
	
	// signed right shift
	assert(i(11501)>>i(5)==i(359));
	assert(i(-11501)>>i(5)==i(-360));
	assert(i(11501)>>i(20)==i(0));
	assert(i(-11501)>>i(20)==i(-1));

	// unsigned addition
	assert(u(10)+u(5)==u(15));

	// unsigned addition with overflow
	assert(u(2000000000)+u(3500000000)==u(1205032704));

	// unsigned subtraction
	assert(u(10)-u(5)==u(5));

	// unsigned subtraction with overflow
	assert(u(5)-u(10)==u(4294967291));

	// unsigned multiplication
	assert(u(10)*u(5)==u(50));

	// unsigned multiplication with overflow
	assert(u(100000)*u(80000)==u(3705032704));

	// unsigned division (dividable)
	assert(u(10)/u(5)==u(2));

	// unsigned division (non dividable)
	assert(u(13)/u(5)==u(2));
	assert(u(10)/u(13)==u(0));

	// unsigned modulo (dividable)
	assert(u(10)%u(5)==u(0));

	// unsigned modulo (non dividable)
	assert(u(13)%u(5)==u(3));
	assert(u(10)%u(13)==u(10));

	// unsigned left shift
	assert(u(13)<<u(5)==u(416));
	#ifndef IX86
		// produces different results at x32 and ix86!, use -DIX86 when 
		//  compiling on gcc
		assert(u(13)<<u(32)==u(0));
	#endif

	// unsigned right shift
	assert(u(11501)>>u(5)==u(359));
	assert(u(11501)>>u(20)==u(0));
}


