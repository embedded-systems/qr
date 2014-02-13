#include <assert.h>

static int i(int i) { return i; }
static unsigned int u(unsigned int u) { return u; }

void test_comp() {
	// signed gt
	assert(i(10)>i(5)==1);
	assert(i(5)>i(10)==0);
	assert(i(5)>i(5)==0);
	assert(i(-10)>i(5)==0);
	assert(i(-5)>i(10)==0);
	assert(i(-5)>i(5)==0);
	assert(i(10)>i(-5)==1);
	assert(i(5)>i(-10)==1);
	assert(i(5)>i(-5)==1);
	assert(i(-10)>i(-5)==0);
	assert(i(-5)>i(-10)==1);
	assert(i(-5)>i(-5)==0);

	// signed ge
	assert(i(10)>=i(5)==1);
	assert(i(5)>=i(10)==0);
	assert(i(5)>=i(5)==1);
	assert(i(-10)>=i(5)==0);
	assert(i(-5)>=i(10)==0);
	assert(i(-5)>=i(5)==0);
	assert(i(10)>=i(-5)==1);
	assert(i(5)>=i(-10)==1);
	assert(i(5)>=i(-5)==1);
	assert(i(-10)>=i(-5)==0);
	assert(i(-5)>=i(-10)==1);
	assert(i(-5)>=i(-5)==1);

	// signed lt
	assert(i(10)<i(5)==0);
	assert(i(5)<i(10)==1);
	assert(i(5)<i(5)==0);
	assert(i(-10)<i(5)==1);
	assert(i(-5)<i(10)==1);
	assert(i(-5)<i(5)==1);
	assert(i(10)<i(-5)==0);
	assert(i(5)<i(-10)==0);
	assert(i(5)<i(-5)==0);
	assert(i(-10)<i(-5)==1);
	assert(i(-5)<i(-10)==0);
	assert(i(-5)<i(-5)==0);

	// signed le
	assert(i(10)<=i(5)==0);
	assert(i(5)<=i(10)==1);
	assert(i(5)<=i(5)==1);
	assert(i(-10)<=i(5)==1);
	assert(i(-5)<=i(10)==1);
	assert(i(-5)<=i(5)==1);
	assert(i(10)<=i(-5)==0);
	assert(i(5)<=i(-10)==0);
	assert(i(5)<=i(-5)==0);
	assert(i(-10)<=i(-5)==1);
	assert(i(-5)<=i(-10)==0);
	assert(i(-5)<=i(-5)==1);

	// signed eq
	assert(i(10)==i(5)==0);
	assert(i(5)==i(10)==0);
	assert(i(5)==i(5)==1);
	assert(i(-10)==i(5)==0);
	assert(i(-5)==i(10)==0);
	assert(i(-5)==i(5)==0);
	assert(i(10)==i(-5)==0);
	assert(i(5)==i(-10)==0);
	assert(i(5)==i(-5)==0);
	assert(i(-10)==i(-5)==0);
	assert(i(-5)==i(-10)==0);
	assert(i(-5)==i(-5)==1);

	// signed ne
	assert(i(10)!=i(5)==1);
	assert(i(5)!=i(10)==1);
	assert(i(5)!=i(5)==0);
	assert(i(-10)!=i(5)==1);
	assert(i(-5)!=i(10)==1);
	assert(i(-5)!=i(5)==1);
	assert(i(10)!=i(-5)==1);
	assert(i(5)!=i(-10)==1);
	assert(i(5)!=i(-5)==1);
	assert(i(-10)!=i(-5)==1);
	assert(i(-5)!=i(-10)==1);
	assert(i(-5)!=i(-5)==0);

	// unsigned gt
	assert(u(10)>u(5)==1);
	assert(u(5)>u(10)==0);
	assert(u(5)>u(5)==0);

	// unsigned ge
	assert(u(10)>=u(5)==1);
	assert(u(5)>=u(10)==0);
	assert(u(5)>=u(5)==1);

	// unsigned lt
	assert(u(10)<u(5)==0);
	assert(u(5)<u(10)==1);
	assert(u(5)<u(5)==0);

	// unsigned le
	assert(u(10)<=u(5)==0);
	assert(u(5)<=u(10)==1);
	assert(u(5)<=u(5)==1);

	// unsigned eq
	assert(u(10)==u(5)==0);
	assert(u(5)==u(10)==0);
	assert(u(5)==u(5)==1);

	// unsigned ne
	assert(u(10)!=u(5)==1);
	assert(u(5)!=u(10)==1);
	assert(u(5)!=u(5)==0);

}
