#define SMALL_LOADER

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <x32.h>
#include <ctype.h>
#include <setjmp.h>
#include "shell.h"
#include "loader_helper.h"

// address of main funcion in a program
#define MAIN_OFFSET 0x60
// loader version
#define LOADER_VERSION "0.3"
// out of memory execution level
#define OOM_LEVEL 0xF002
// trap execution level
#define TRAP_LEVEL 0xF001
// division by 0 execution level
#define DIV0_LEVEL 0xF000

// welcome message
#define WELCOME "\
X32 Bootloader (c) 2005-2006 TU Delft\r\n\
  Sijmen Woutersen, Arjan van Gemund, Mark Dufour\r\n\
"


// main menu
#define MENU "\
  b address                        Set breakpoint\r\n\
  c                                Continue\r\n\
  d from_addr to_addr [f|h|b]      Download memory\r\n\
  m                                Show this menu\r\n\
  p index [value]                  Get/set peripheral value\r\n\
  s [address] [j]                  Start program\r\n\
  u from_addr to_addr [h|b]        Upload memory\r\n\
  r                                Restart\r\n\
  v                                Show version\r\n\
"

TERMINAL main_term;

int parse_hex(const char*, unsigned long*);

int command_loop(int);

void print_command_usage(char);
void print_version();
void dump_memory_formatted(unsigned long, unsigned long);
void dump_memory_hex(unsigned long, unsigned long);
void dump_memory_binary(unsigned long, unsigned long);
void load_memory_hex(unsigned long, unsigned long, int);
void load_memory_binary(unsigned long, unsigned long, int);
void call_program(unsigned long);
void set_trap(unsigned long);
void reset_interrupts();

jmp_buf env;

const char b_usage[] = "Usage: b address\r\n";
const char d_usage[] = "Usage: d address1 address2 [f|h|b]\r\n";
const char u_usage[] = "Usage: u address1 address2 [h|b]\r\n";
const char s_usage[] = "Usage: s address [j]\r\n";
const char p_usage[] = "Usage: p index [value]\r\n";
const char n_usage[] = "Usage: n [0|1]\r\n";
const char invalid_command[] = "Unknown command, type 'm' for menu\r\n";

/* nub variables */
unsigned long *fp;
char okay[] = "\aDBG okay\r\n";
int nub_mode;

int main() {
	// initialize interrupts
	reset_interrupts();

	// create a terminal on putchar & getchar
	open_terminal(&main_term);

	printf(WELCOME);
	printf("\r\n");
	
	// reset nubmode
	nub_mode = 0;
	while(1) command_loop(0);

	return 0;
}

int command_loop(int debug) {
	int exit_loop = 0;
    int interrupt_enable;
	int i;
	unsigned long v1, v2;

	if (nub_mode) {
		printf(okay); 
	} else {
		printf(MENU);
	}

	if (!nub_mode) printf("\r\n");

	while(!exit_loop) {
		if (nub_mode) {
			// no prompt in nubmode
		} else if (debug) {
			printf("[trapped]=> ");
		} else {
			printf("=> ");
		}

		get_command(&main_term);

		// check if a command has been given
		if (main_term.arg_count > 0) {

			if (main_term.args[0][1] == '\0') {
				// 1 character long commands:
				switch(tolower(main_term.args[0][0])) {
					case 'v':
						print_version(&main_term);
						break;
					case 'd':
						if (main_term.arg_count == 3) {
							if (parse_hex(main_term.args[1], &v1) && 
								parse_hex(main_term.args[2], &v2) &&
								v1 <= v2) {

								dump_memory_formatted(v1, v2-1);
							} else {
								printf(d_usage);
							}
						} else if (main_term.arg_count == 4) {
							if (parse_hex(main_term.args[1], &v1) && 
								parse_hex(main_term.args[2], &v2) &&
								v1 <= v2) {

								if (main_term.args[3][1] == '\0') {
									switch (tolower(main_term.args[3][0])) {
										case 'f': // formatted
											dump_memory_formatted(v1, v2-1);
											break;
										case 'h': // hexadecimal string
											dump_memory_hex(v1, v2-1);
											break;
										case 'b': // binary stream
											dump_memory_binary(v1, v2-1);
											break;
									}
								} else {
									printf(d_usage);
								}
							} else {
								printf(d_usage);
							}
						} else {
							printf(d_usage);
						}
						break;
					case 'u':
						if (main_term.arg_count == 3) {
							if (parse_hex(main_term.args[1], &v1) && 
								parse_hex(main_term.args[2], &v2) &&
								v1 <= v2) {

								load_memory_hex(v1, v2-1, 1);
							} else {
								printf(u_usage);
							}
						} else if (main_term.arg_count == 4) {
							if (parse_hex(main_term.args[1], &v1) && 
								parse_hex(main_term.args[2], &v2) &&
								v1 <= v2) {

								if (main_term.args[3][1] == '\0') {
									switch (tolower(main_term.args[3][0])) {
										case 'h': // hexadecimal string
											load_memory_hex(v1, v2-1, 1);
											if (nub_mode) printf(okay);
											break;
										case 'b': // binary stream
											load_memory_binary(v1, v2-1, 1);
											break;
									}
								} else {
									printf(u_usage);
								}
							} else {
								printf(u_usage);
							}
						} else {
							printf(u_usage);
						}
						break;
					case 'n':
						if (main_term.arg_count == 2) {
							if (main_term.args[1][1] == '\0' && main_term.args[1][0] == '1') {
								printf("WARNING: Switching to nub mode. To switch back, type 'n 0[enter]'.\r\n");
								printf(okay);
								nub_mode = 1;
							} else if (main_term.args[1][1] == '\0' && main_term.args[1][0] == '0') {
								nub_mode = 0;
							} else {
								printf(n_usage);
							}
						} else {
								printf(n_usage);
						}
						break;
					case 'b':
						if (main_term.arg_count == 2) {
							if (parse_hex(main_term.args[1], &v1)) {
								set_trap(v1);
								if(nub_mode) {
									printf(okay);
								} else {
									printf("Breakpoint set at 0x%08X\r\n", v1);
								}
							} else {
								printf(b_usage);
							}
						} else {
							printf(b_usage);
						}
						break;
					case 'p':
						if (main_term.arg_count == 2) {
							if (parse_hex(main_term.args[1], &v1)) {
								printf("peripherals[0x%X]: 0x%08X\r\n", v1, peripherals[v1]);
							} else {
								printf(p_usage);
							}
						} else if (main_term.arg_count == 3) {
							if (parse_hex(main_term.args[1], &v1) && 
								parse_hex(main_term.args[2], &v2)) {

								peripherals[v1] = v2;
								printf("peripherals[0x%X]: 0x%08X\r\n", v1, v2);

							} else {
								printf(p_usage);
							}
						} else {
							printf(p_usage);
						}
						break;
					case 's':
						if (!debug) {
							if (main_term.arg_count == 1) {
								call_program(0);
							} else if (main_term.arg_count == 2) {
								if (tolower(main_term.args[1][0]) == 'j' && 
									main_term.args[1][1] == '\0') { 
	
									jump(0);
								} else if (parse_hex(main_term.args[1], &v1)) {
									call_program(v1);
								} else {
									printf(s_usage);
								}
							} else if (main_term.arg_count == 3) {
								if (tolower(main_term.args[1][0]) == 'j' && 
									main_term.args[1][1] == '\0') { 
	
									if (parse_hex(main_term.args[2], &v1)) {
										jump((void*)v1);
									} else {
										printf(s_usage);
									}
								} else if (tolower(main_term.args[2][0]) == 'j' && 
									main_term.args[2][1] == '\0') { 
	
									if (parse_hex(main_term.args[1], &v1)) {
										jump((void*)v1);
									} else {
										printf(s_usage);
									}
								} else {
									printf(s_usage);
								}
							} else {
								printf(s_usage);
							}
						} else {
							printf("Program already running\r\n");
						}
						break;
					case 'm': case 'h':
						printf(MENU);
						break;
					case 'c':
						if (debug) {
							exit_loop = 1;
						} else {
							printf("No program running, can't continue\r\n");
						}
						break;
					case 'x':
						if (nub_mode) printf("\aDBG %08X\r\n", fp);
						break;
					case 'r':
						printf("WARNING: This is only a software reset.\r\n");
						printf("  To completely restart the x32, use the hardware reset button on the x32 board.\r\n");
						set_execution_level(0);
						nub_mode = 0;
						fp = 0;
						jump(start_address());
					default:
						printf(invalid_command);
						break;
				}
			} else {
				// > 1 character long commands:
				printf(invalid_command);
			}

		}
		if (!nub_mode) printf("\r\n");

	}

	/* clear pending interrupts before returning */
	#ifdef PERIPHERAL_INT_ENABLE
		set_execution_level(THREAD_SAFE_EXECUTION_LEVEL);
		interrupt_enable = peripherals[PERIPHERAL_INT_ENABLE];
		peripherals[PERIPHERAL_INT_ENABLE] = 0;
		peripherals[PERIPHERAL_INT_ENABLE] = interrupt_enable;
	#endif
	return 0;
}

void print_version() {
	printf("Loader version:  %s\r\n", LOADER_VERSION);
	printf("Core version id: 0x%08X\r\n", get_version());
	printf("Peripheral id:   0x%08X\r\n", peripherals[0]);
}

void dump_memory_formatted(unsigned long from_addr, unsigned long to_addr) {
	unsigned long i, j, addr;
	unsigned char* memory = 0;
	
	for (i = (from_addr >> 4); i < ((to_addr + 0x10) >> 4); i++) {
		printf("%07X0: ", i);
		
		for (j = 0; j < 0x10; j++) {
			addr = (i<<4) | j;
			if ((addr < from_addr) || (addr > to_addr)) {
				printf("   ");
			} else {
				printf("%02X ", memory[addr]);
			}
		}
		
		printf(": ");
		
		for (j = 0; j < 0x10; j++) {
			addr = (i<<4) | j;
			if ((addr < from_addr) || (addr > to_addr)) {
				printf(" ");
			} else if (isprint(memory[addr])) {
				printf("%c", memory[addr]);
			} else {
				printf(".");
			}
		}
		
		printf("\r\n");
	}
}

void dump_memory_hex(unsigned long from_addr, unsigned long to_addr) {
	unsigned char* memory = 0;
	unsigned long i;

	if (nub_mode) printf("\aDBG ");
	for (i = from_addr; i <= to_addr; i++) printf("%02X", memory[i]);
	if (nub_mode) printf("\r\n");
}

void dump_memory_binary(unsigned long from_addr, unsigned long to_addr) {
	unsigned char* memory = 0;
	unsigned long i;

	for (i = from_addr; i <= to_addr; i++) putchar(memory[i]);
}

void load_memory_hex(unsigned long from_addr, unsigned long to_addr, int echo) {
	unsigned char* memory = 0;
	unsigned long i;
	unsigned char c;
	unsigned char v;

	for (i = from_addr; i <= to_addr; i++) {
		while (1) {
			c = getchar();
			if (isdigit(c)) {
				v = c-'0';
				if (echo && !nub_mode) putchar(c);
				break;
			} else if (isxdigit(c)) {
				v = toupper(c)-'A'+10;
				if (echo && !nub_mode) putchar(c);
				break;
			}
		}
		v = v << 4;
		while (1) {
			c = getchar();
			if (isdigit(c)) {
				v |= c-'0';
				if (echo && !nub_mode) putchar(c);
				break;
			} else if (isxdigit(c)) {
				v |= toupper(c)-'A'+10;
				if (echo && !nub_mode) putchar(c);
				break;
			}
		}
		memory[i] = v;
	}
}

void load_memory_binary(unsigned long from_addr, unsigned long to_addr, int echo) {
	unsigned char* memory = 0;
	unsigned long i;
	int c;

	for (i = from_addr; i <= to_addr; i++) {
		while ((c = getchar()) < 0);
		memory[i] = (unsigned char)c;
		if (echo) putchar(memory[i]);
	}
}

void call_program(unsigned long address) {
	int (*main_func)(int, char **);
	char* arg = "application";
	int ret = EXIT_FAILURE;
	unsigned long start, stop;
	
	*(int*)&main_func = address + MAIN_OFFSET;
	start = X32_MS_CLOCK;
	if (!setjmp(env)) ret = main_func(1, &arg);
	stop = X32_MS_CLOCK;
	reset_interrupts();
	set_execution_level(0);
	if(!nub_mode) {
		printf("Program return value: 0x%08X (%d)\r\n", ret, ret);
		printf("Program execution took: %dms\r\n", stop - start);
	} else {
		printf("\aDBG halt\r\n");
		nub_mode = 0;
	}
}

int parse_hex(const char* hex, unsigned long* value) {
	char* str = (char*)hex;
	unsigned long ret = 0;

	while(*str) {
		ret = ret << 4;
		if (isdigit(*str)) {
			ret += *str-'0';
		} else if (isxdigit(*str)) {
			ret += toupper(*str)-'A'+10;
		} else {
			return 0;
		}
		str++;
	}

	*value = ret;
	return 1;
}

/**************************************************************************\
                                 DEBUGGER
\**************************************************************************/
void trap_handler();
void div0_handler();
void oom_handler();
void set_trap(unsigned long);

void reset_interrupts() {
	int i;
#ifdef INTERRUPT_MAX
	for (i = 0; i <= INTERRUPT_MAX; i++) {
		switch(i) {
#ifdef INTERRUPT_TRAP
			case INTERRUPT_TRAP:
				INTERRUPT_VECTOR(INTERRUPT_TRAP) = &trap_handler;
				INTERRUPT_PRIORITY(INTERRUPT_TRAP) = TRAP_LEVEL;
				ENABLE_INTERRUPT(INTERRUPT_TRAP);
				break;
#endif // INTERRUPT_TRAP
#ifdef INTERRUPT_DIV0
			case INTERRUPT_DIVISION_BY_ZERO:
				INTERRUPT_VECTOR(INTERRUPT_DIVISION_BY_ZERO) = &div0_handler;
				INTERRUPT_PRIORITY(INTERRUPT_DIVISION_BY_ZERO) = DIV0_LEVEL;
				ENABLE_INTERRUPT(INTERRUPT_DIVISION_BY_ZERO);
				break;
#endif // INTERRUPT_DIV0
#ifdef INTERRUPT_OUT_OF_MEMORY
			case INTERRUPT_OUT_OF_MEMORY:
				INTERRUPT_VECTOR(INTERRUPT_OUT_OF_MEMORY) = &oom_handler;
				INTERRUPT_PRIORITY(INTERRUPT_OUT_OF_MEMORY) = OOM_LEVEL;
				ENABLE_INTERRUPT(INTERRUPT_OUT_OF_MEMORY);
				break;
#endif // INTERRUPT_OUT_OF_MEMORY
			default:
				DISABLE_INTERRUPT(i);
				break;
		}
	}
#ifdef INTERRUPT_GLOBAL
	DISABLE_INTERRUPT(INTERRUPT_GLOBAL);
#endif // INTERRUPT_GLOBAL
#endif // INTERRUPT_MAX
}

void set_trap(unsigned long address) {
	INSTRUCTION instruction;
	instruction = *(INSTRUCTION*)address;
	instruction |= 0x8000; 
	*(INSTRUCTION*)address = instruction; 
}

#ifdef INTERRUPT_TRAP
	void trap_handler(void) {
		unsigned long* ra;
	
		fp = _get_fp();
	
		ra = (unsigned long*)((unsigned char*)_get_fp()-5*sizeof(int*));
        //printf("r %x\n", *ra);
		*((INSTRUCTION*)(*ra)) = *((INSTRUCTION*)(*ra)) & ~0x8000;
	
		command_loop(1);
	
	}
#endif

#ifdef INTERRUPT_DIV0
	void div0_handler(void) {
		unsigned long* ra;
		ra = (unsigned long*)((unsigned char*)_get_fp()-5*sizeof(int*));
		printf("\r\nDivision by zero at 0x%08X!\r\n", (int)*ra);
		longjmp(env,1);
	}
#endif

#ifdef INTERRUPT_OUT_OF_MEMORY
	void oom_handler(void) {
		unsigned long* ra;
		ra = (unsigned long*)((unsigned char*)_get_fp()-5*sizeof(int*));
		printf("\r\nOut of memory at 0x%08X!\r\n", (int)*ra);
		DISABLE_INTERRUPT(INTERRUPT_OUT_OF_MEMORY);
		longjmp(env,2);
	}
#endif












