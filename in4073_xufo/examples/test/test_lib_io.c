#include <stdio.h>
#include <assert.h>
#include <string.h>


void test_lib_io() {
	char string[100];
	int n;

	assert(sprintf(string, "%d %d %d %04d %4d %-4d %d", 0, 100, -100, 50, 50, 50, 80000)==31);
	assert(strcmp(string, "0 100 -100 0050   50 50   80000")==0);

	assert(sprintf(string, "%i %i %i %04i %4i %-4i %i", 0, 100, -100, 50, 50, 50, 80000)==31);
	assert(strcmp(string, "0 100 -100 0050   50 50   80000")==0);

	assert(sprintf(string, "%o %o %o %04o %4o %-4o %o", 0, 0100, -0100, 050, 050, 050, 070000)>0);
	assert(strcmp(string, "0 100 37777777700 0050   50 50   70000")==0);

	assert(sprintf(string, "%x %x %x %04x %4x %-4x %x", 0, 0x10a, -0x10b, 0x5c, 0x5D, 0x5E, 0x8000F)==35);
	assert(strcmp(string, "0 10a fffffef5 005c   5d 5e   8000f")==0);

	assert(sprintf(string, "%X %X %X %04X %4X %-4X %X", 0, 0x10a, -0x10b, 0x5c, 0x5D, 0x5E, 0x8000F)==35);
	assert(strcmp(string, "0 10A FFFFFEF5 005C   5D 5E   8000F")==0);

	assert(sprintf(string, "%u %u %u %04u %4u %-4u %u", 0, 100, -100, 50, 50, 50, 80000)==37);
	assert(strcmp(string, "0 100 4294967196 0050   50 50   80000")==0);
	
	assert(sprintf(string, "%s %s %c %c %s %s", "test1", "test2", '0', 'A', "test3", "test4")==27);
	assert(strcmp(string, "test1 test2 0 A test3 test4")==0);

	assert(sprintf(string, "%d %d %d %d [%n] %d %d %d", 1, 2, 3, 4, &n, 6, 7, 8)==16);
	assert(strcmp(string, "1 2 3 4 [] 6 7 8")==0);
	assert(n==9);
}
