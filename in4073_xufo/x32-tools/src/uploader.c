#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bool.h"
#include "upload_lib.h"
#include "serial.h"

void print_usage();
int hex2bin(char*);
void* comhandle;
void rs232_putchar(char);
int rs232_getchar(); 

/* option execute_by_call
 *		when set to non zero, the uploader will start a program
 *		by making a call to address 0x60, otherwise, it will make
 *		a jump to address 0x00 (both increased by the upload
 *		location). In general a call is safer, however it 
 *		requires the program to be linked with the new bootstrap
 *		code included in v0.1.1 of the toolset, which contains
 *		a wrapper for the main function at location 0x60. Programs
 *		compiled with the older bootstrap code (<april 2006) will
 *		not have the wrapper, and calling address 0x60 will thus
 *		fail (note: the old bootstrap code had zeroes placed at
 *		address 0x60, and a call will thus result in a processor
 *		halt or reset).
 */
int execute_by_call = 1;

int main(int argc, char** argv) {
	BOOL is_loc, is_com, auto_exec;
	int location, i;
	char* filename;
	char* comport;
	char message[16];
	
	is_loc = is_com = auto_exec = FALSE;
	location = 0;
	filename = comport = 0;
	
	for (i = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			/* switch */
			if (strcmp(&argv[i][1], "l") == 0) {
				is_loc = TRUE;
			} else if (strcmp(&argv[i][1], "c") == 0) {
				is_com = TRUE;
			} else if (strcmp(&argv[i][1], "e") == 0) {
				auto_exec = TRUE;
			} else {
				printf("Invalid switch %s\n", argv[i]);
				print_usage();
				exit(EXIT_FAILURE);
			}
		} else {
			if (is_loc) {
				location = hex2bin(argv[i]);
				is_loc = FALSE;
			} else if (is_com) {
				comport = argv[i];
				is_com = FALSE;
			} else {
				filename = argv[i];
			}
		}
	}
	
	/* check filename */
	if (filename == 0) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	/* check comport */
	if (comport == 0) {
		print_usage();
		exit(EXIT_FAILURE);
	}
	
	/* open comm */
	if ((comhandle = rs232_open(comport, 0)) == 0) {
		printf("Could not open '%s'\n", comport);
		exit(EXIT_FAILURE);
	}
	
	/* set fpga communication functions */
	set_putchar(&rs232_putchar);
	set_getchar(&rs232_getchar);

	if (upload_file(filename, location, 1, 0) == 0) {
		printf("Upload failed\n");
		rs232_close(comhandle);
		exit(EXIT_FAILURE);
	} else if (auto_exec) {
		printf("Starting application...\n");
		if (execute_by_call) {
			sprintf(message, "s %08X\0", location);
		} else {
			sprintf(message, "s %08X j\0", location);
		}
		exec_command(message);
		rs232_close(comhandle);
	}
	
	return 0;
}

/*
 * print usage to screen
 */
void print_usage() {
	printf("Usage: \n");
	printf("\tx32-upload filename -c comport [-l location] [-e]\n\n");
	printf("\tcomport:      name of comport (/dev/ttyS0, com1, ...)\n");
	printf("\tlocation:     hexadecimal value of the position to store the program\n");
	printf("\te:            automatically start the program after upload\n");
}

/* 
 * convert hexadecimal string to integer
 */
int hex2bin(char* hex) {
	int i, ret;
	i = ret = 0;
	while(hex[i]) {
		ret <<= 4;
		if (hex[i] >= '0' & hex[i] <= '9') {
			ret += hex[i] - '0';
		} else if (hex[i] >= 'A' & hex[i] <= 'F') {
			ret += hex[i] - 'A' + 10;
		} else if (hex[i] >= 'a' & hex[i] <= 'f') {
			ret += hex[i] - 'a' + 10;
		} else {
			return -1;
		}
		i++;
	}
	return ret;
}

int rs232_getchar() {
	unsigned char byte;
	if (rs232_read(comhandle, &byte, 1)) {
		return (int)byte;
	} else {
		return -1;
	}
}

void rs232_putchar(char byte) {
	rs232_write(comhandle, &byte, 1);
}



