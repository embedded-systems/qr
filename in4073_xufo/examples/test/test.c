#include <stdlib.h>
#include <stdio.h>
#ifndef IX86
#include <x32.h>
#endif

void test_math();
void test_comp();
void test_array_struct();
void test_lib_string();
void test_lib_std_setjmp();
void test_lib_io();
void test_alu_errors();
void test_trap();
void test_oom();

int main() {
	puts("Testing mathematics...");
	test_math();
	puts("OK!");
	
	puts("Testing comparisons...");
	test_comp();
	puts("OK!");

	puts("Testing comparisons...");
	test_array_struct();
	puts("OK!");

	puts("Testing string library...");
	test_lib_string();
	puts("OK!");

	puts("Testing alu errors...");
	test_alu_errors();
	puts("OK!");

	puts("Testing assert, standard & setjmp library...");
	test_lib_std_setjmp();
	puts("OK!");

	puts("Testing I/O library");
	test_lib_io();
	puts("OK!");

	puts("Testing trap instruction...");
	test_trap();
	puts("OK!");

	puts("Testing out of memory protection...");
	test_oom();
	puts("OK!");

	puts("No errors found");

	return 0;
}
