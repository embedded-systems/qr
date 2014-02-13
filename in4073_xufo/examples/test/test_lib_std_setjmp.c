#include <stdlib.h>
#include <assert.h>
#include <setjmp.h>
#include <limits.h>

// these are required by the x32 malloc functions
int malloc_memory_size = 1024;
char malloc_memory[1024];

int atexit_counter;
int protect;

jmp_buf env;

void atexit1() {
	atexit_counter = 1;
}

void atexit2() {
	if (protect) longjmp(env,10);
}

void test_lib_std_setjmp() {
	void* memory[10];
	int i;
	char *str1, *str2;
	ldiv_t ld;
	div_t d;

	// strtol, atoi, atol
	str1 = "111";
	assert(strtol(str1, 0, 2)==7);
	assert(strtol(str1, 0, 8)==73);
	assert(strtol(str1, 0, 10)==111);
	assert(atoi(str1)==111);
	assert(atol(str1)==111);
	assert(strtol(str1, 0, 16)==273);
	str1 = "15A";
	assert(strtol(str1, &str2, 2)==1);
	assert(str2==str1+1);
	assert(strtol(str1, &str2, 8)==13);
	assert(str2==str1+2);
	assert(strtol(str1, &str2, 10)==15);
	assert(str2==str1+2);
	assert(atoi(str1)==15);
	assert(atol(str1)==15);
	assert(strtol(str1, &str2, 16)==346);
	str1 = "100000000";
	assert(strtol(str1, 0, 16)==LONG_MAX);
	str1 = "-100000000";
	assert(strtol(str1, 0, 16)==LONG_MIN);
	str1 = "-1";
	assert(strtol(str1, 0, 16)==-1);
	str1 = "+1";
	assert(strtol(str1, 0, 16)==1);

	// strtoul
	str1 = "111";
	assert(strtoul(str1, 0, 2)==7);
	assert(strtoul(str1, 0, 8)==73);
	assert(strtoul(str1, 0, 10)==111);
	assert(strtoul(str1, 0, 16)==273);
	str1 = "15A";
	assert(strtoul(str1, &str2, 2)==1);
	assert(str2==str1+1);
	assert(strtoul(str1, &str2, 8)==13);
	assert(str2==str1+2);
	assert(strtoul(str1, &str2, 10)==15);
	assert(str2==str1+2);
	assert(strtoul(str1, &str2, 16)==346);
	str1 = "100000000";
	assert(strtoul(str1, 0, 16)==ULONG_MAX);
	str1 = "-1";
	assert(strtoul(str1, 0, 16)==ULONG_MAX);
	str1 = "+1";
	assert(strtoul(str1, 0, 16)==1);

	// abs
	assert(abs(10)==10);
	assert(abs(-10)==10);
	assert(labs(10)==10);
	assert(labs(-10)==10);

	// div & ldiv
	d = div(100,7);
	assert(d.quot==14);
	assert(d.rem==2);
	ld = ldiv(100,7);
	assert(ld.quot==14);
	assert(ld.rem==2);

	#ifndef IX86
		// malloc, calloc & free
		memory[0] = malloc(256);
		memory[1] = calloc(16,16);
		memory[2] = malloc(256);
		memory[3] = malloc(256);
		memory[4] = malloc(128);
		memory[5] = malloc(64);
		memory[6] = malloc(64);
		free(memory[0]);
		memory[7] = malloc(64);
		memory[8] = malloc(64);
		memory[9] = malloc(64);


		assert(memory[0]==malloc_memory+0x0004);
		assert(memory[1]==malloc_memory+0x0108);
		assert(memory[2]==malloc_memory+0x020C);
		assert(memory[3]==(void*)0x0000);
		assert(memory[4]==malloc_memory+0x0310);
		assert(memory[5]==malloc_memory+0x0394);
		assert(memory[6]==(void*)0x0000);
		assert(memory[7]==malloc_memory+0x0004);
		assert(memory[8]==malloc_memory+0x0048);
		assert(memory[9]==malloc_memory+0x008C);

		for (i = 0; i < 16*4; i++) assert(((int*)(memory[1]))[i]==0);

		free(memory[1]);
		free(memory[2]);
		free(memory[4]);
		free(memory[5]);
		free(memory[7]);
		free(memory[8]);
		free(memory[9]);

		// exit, atexit, abort
		#ifndef DURA
			// these functions can only be tested once, skip them in dura test
			protect = 1;
			atexit_counter = 0;
			atexit(atexit2);
			atexit(atexit1);
		
			i = setjmp(env);
			if (i) {
				assert(i==10);
				assert(atexit_counter==1);
			} else {
				exit(5);
			}
		
			i = setjmp(env);
			if (!i) abort();
		
			i = setjmp(env);
			if (!i) assert("Testing assert..."==0);
	
			protect = 0;
		#endif
	#else
		puts("\tMalloc & exit function tests skipped");
	#endif

}



