#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serial.h"
#include "io.h"
#include "debug_decoder.h"
#include "instructions.h"

/* file read buffer size */
#define BUFF_SIZE 1024

/* simple buffer structure for buffering rs232 data */
typedef struct {
	unsigned char buffer[1024];	/* the buffer */
	int size;										/* the amount of unread bytes in the buffer */
	int start;									/* the first unread byte in the buffer */
} BUFFER;

/* function prototypes */
/* print program usage (command line options etc) */
void printusage();
/* update the rs232 buffer (add/remove bytes).
		void* is a file pointer to the rs232 device
		BUFFER* is a pointer to a BUFFER structure which contains the buffer
		int, if positive: make sure this many bytes are available in the buffer
			(read from rs232 if not enough bytes available)
		int, if negative: remove this many bytes from the buffer
*/
int updatebuffer(void*, BUFFER*, int);
/* load a file into the memory
		char* is a filename
		unsigned char* is a pointer to the memory array
		int is a positive number specifying the offset to store the file
		int is a positive number specifying the size of the memory

		returns 0 (success), 1 (file not found) or 2 (out of memory)
*/
int load_file(char*, unsigned char*, int, int);
/* convert hexadecimal string to integer */
int hex2bin(char*);

/* program entry point */
int main(int argc, char** argv) {
	void* serial; 								/* file pointer to rs232 device */
	char* comport;								/* char pointer holding the device name */
	char* programfile;						/* char pointer holding the program filename */
	unsigned char outbuffer[4];		/* rs232 output buffer */
	int i;												/* variable used in various loops */
	int synchronized;							/* protocol state, if synchronized */
	int booting;									/* indicates the processor is loading the 
																		code image from its ROM */
	int size;											/* memory size (in KB) */
	int verbose;									/* verbose mode */
	int confirm;									/* confirmation mode */
	int count;										/* number of bytes reading/writing */
	int address;									/* memory address */
	int sign_extend;							/* indicates the data is 2's complement */
	unsigned char* memory;				/* memory array */
	int state, substate;					/* states used by the debugger */
	char message[100];						/* message space for debugger */
	BUFFER inbuffer;							/* rs232 input buffer */
	int read_overrule;						/* read overrule flag, allows overrulling the
																		data send to the processor to force a jump
																		to 0 when executing a program from the 
																		rs232 memory instead of the processors 
																		rom */

	/* default values */
	comport = programfile = 0;
	memory = 0;
	verbose = confirm = 0;
	size = 1024; // in KB

	/* parse command line */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][2] == '\0') {
			switch (argv[i][1]) {
				case 'c':
					comport = argv[i+1];
					i++;
					break;
				case 's':
					if (memory) {
						printf("Only one size parameter is allowed");
						printusage();
					} else {
						sscanf("%d", argv[i+1], size);
						/* allocate memory */
						memory = (unsigned char*)malloc(size*1024);
					}
					i++;
					break;
				case 'f':
					if ((count = hex2bin(argv[i+2])) < 0) {
						printf("Invalid target location for %s\n", argv[i+1]);
						printusage();
					} else if (!memory) {
						printf("-s parameter must preceede any -f parameters\n");
						printusage();
					} else {
						switch (load_file(argv[i+1], memory, count, size*1024)) {
							case 0:
								printf("%s loaded at 0x%X\n", argv[i+1], count);
								break;
							case 1:
								printf("Could not open %s\n", argv[i+1]);
								break;
							case 2:
								printf("Not enough memory to load %s\n", argv[i+1]);
								break;
						}
					}
					i+=2;
					break;
				case 'p':
					programfile = argv[i+1];
					i++;
					break;
				case 'v':
					verbose = 1;
					break;
				case 'a':
					confirm = 1;
					break;
				default:
					printf("Unknown argument '%s'\n", argv[i]);
					printusage();
					break;
			}
		} else {
			printf("Unknown argument '%s'\n", argv[i]);
			printusage();
		} 
	}

	/* check comport */
	if (comport == 0) {
		printf("No comport specified\n");
		printusage();
		exit(EXIT_FAILURE);
	}

	/* check size */
	if (size < 1) {
		printf("Memory size must be at least 1KB\n");
		printusage();
		exit(EXIT_FAILURE);
	}

	/* check memory */
	if (memory == 0) {
		memory = (unsigned char*)malloc(size*1024);
	}

	/* open serial port (nonblocking) */
	serial = rs232_open(comport, 0);
	if (serial == 0) {
		printf("Could not open serial port!\n");
		exit(EXIT_FAILURE);
	}
	
	/* initialize */
	booting = 1;
	synchronized = 0;
	read_overrule = 0;
	printf("Listening on %s\n", comport);
	inbuffer.size = inbuffer.start = 0;

	/* run forever */
	while(1) {
		/* need 1 byte */
		updatebuffer(serial, &inbuffer, 1);
		
		/* handle command */
		if (inbuffer.buffer[inbuffer.start] == 0xFF) {
			printf("Synchronizing: ");
			/* need 5 bytes */
			updatebuffer(serial, &inbuffer, 5);

			/* check sync code */
			if (inbuffer.buffer[inbuffer.start+1] == 0x0F && 
				inbuffer.buffer[inbuffer.start+2] == 0x1E && 
				inbuffer.buffer[inbuffer.start+3] == 0x2D && 
				inbuffer.buffer[inbuffer.start+4] == 0x3C) {

				/* reply the synchronization code */
				outbuffer[0] = 0x0F; outbuffer[1] = 0x1E;
				outbuffer[2] = 0x2D; outbuffer[3] = 0x3C;
				rs232_write(serial, outbuffer, 4);

				/* remove 5 bytes */
				updatebuffer(serial, &inbuffer, -5);

				/* mark server as synchronized */
				synchronized = 1;
				/* probable proc reset result in proc booting, assume it is */
				booting = 1;
				/* reset the debugger */
				state = substate = 0;				

				printf("OK => PROCESSOR BOOTING\n");
			} else {
				/* remove 1 byte */
				updatebuffer(serial, &inbuffer, -1);

				/* synchronization failed */
				synchronized = 0;
				printf("FAILED\n");
			}
		} else if (!synchronized) {
			/* only the synchronize command is accepted when not synchronized */
			printf("Unsynchronized [%02X]\n", inbuffer.buffer[inbuffer.start]);
			/* remove 1 byte */
			updatebuffer(serial, &inbuffer, -1);
			synchronized = 0;
		} else if ((inbuffer.buffer[inbuffer.start] & 0xC0) != 0x40) {
			/* first to bits must be 01 */
			printf("Invalid command (1) [%02X]\n", inbuffer.buffer[inbuffer.start]);
			/* remove 1 byte */
			updatebuffer(serial, &inbuffer, -1);
			synchronized = 0;
		} else if ((inbuffer.buffer[inbuffer.start] & 0x30) == 0x10) {
			/* WRITE command */
			/* need 9 bytes */
			updatebuffer(serial, &inbuffer, 9);

			/* decode signed & size bits */
			sign_extend = inbuffer.buffer[inbuffer.start] & 0x08;
			switch(inbuffer.buffer[inbuffer.start] & 0x07) {
				case 0: count = 0; break; // void
				case 1: count = 1; break; // char
				case 2: count = 2; break; // short
				case 3: count = 4; break; // int
				case 4: count = 4; break; // long
				case 5: count = 4; break; // long long (no longer supported)
				case 6: count = 4; break; // pointer
				case 7: count = 2; break; // instruction
			}

			/* decode address */
			address = (inbuffer.buffer[inbuffer.start+1] << 24) + 
				(inbuffer.buffer[inbuffer.start+2] << 16) + 
				(inbuffer.buffer[inbuffer.start+3] << 8) + 
				inbuffer.buffer[inbuffer.start+4];

			/* check memory boundaries */
			if (address+count > size*1024 || address < 0) {
				printf("Out of memory, could not write %d bytes to 0x%08X!\n", 
					count, address);
			} else {
				if (verbose) printf("Writing %d bytes to   0x%08X [", count, address);
				for (i = 0; i < count; i++) {
					memory[address+i] = inbuffer.buffer[inbuffer.start+5+i+4-count];
					if (verbose) 
						printf("%02X ", inbuffer.buffer[inbuffer.start+5+i+4-count]);
				}
				if (verbose) printf("\b]: ");
	
				/* print debug info */
				if (verbose && !booting) {
					if (i = action_text(&state, &substate, ACTION_WRITE, outbuffer, 
						message)) {

						printf(message);
					} else if (!booting) {
						printf("Unknown processor action");
					}
					if (confirm) {
						getchar();
					} else {
						printf("\n");
					}
				} else if (verbose) {
					printf("Loading code image\n");
				}

				/* echo command byte as acknowledge (used to slow down burst writes) */
				outbuffer[0] = inbuffer.buffer[inbuffer.start];
				rs232_write(serial, outbuffer, 1);
			}


			/* remove 9 bytes */
			updatebuffer(serial, &inbuffer, -9);
		} else if ((inbuffer.buffer[inbuffer.start] & 0x30) == 0x20) {
			/* need 5 bytes */
			updatebuffer(serial, &inbuffer, 5);

			/* when reading, the proc is surely not booting */
			if (booting) {
				printf("Code image loaded\n");
				if (programfile) {
					printf("Loading program file... ");
					switch (load_file(programfile, memory, 0, size*1024)) {
						case 0:
							printf("OK\n");
							break;
						case 1:
							printf("FAILED: Could not open file\n");
							break;
						case 2:
							printf("FAILED: Out of memory\n");
							break;
					}
					read_overrule = 1;
				}
			}
			booting = 0;
			
			/* decode signed & size bits */
			sign_extend = inbuffer.buffer[inbuffer.start] & 0x08;
			switch(inbuffer.buffer[inbuffer.start] & 0x07) {
				case 0: count = 0; break; // void
				case 1: count = 1; break; // char
				case 2: count = 2; break; // short
				case 3: count = 4; break; // int
				case 4: count = 4; break; // long
				case 5: count = 4; break; // long long (no longer supported)
				case 6: count = 4; break; // pointer
				case 7: count = 2; break; // instruction
			}

			/* decode address */
			address = (inbuffer.buffer[inbuffer.start+1] << 24) + 
				(inbuffer.buffer[inbuffer.start+2] << 16) + 
				(inbuffer.buffer[inbuffer.start+3] << 8) + 
				inbuffer.buffer[inbuffer.start+4];
	
			if (address+count > size*1024 || address < 0) {
				printf("Out of memory, could not read %d bytes from 0x%08X!\n", 
					count, address);
			} else {

				if (verbose) printf("Reading %d bytes from 0x%08X [", count, address);

				/* read the data, and write to output buffer */
				switch(read_overrule) {
					case 0: // read from memory
						for (i = 0; i < 4; i++) {
							if (4-i > count) {
								if (sign_extend && (memory[address] & 0x80)) {
									outbuffer[i] = 0xFF;
								} else {
									outbuffer[i] = 0x00;
								}
							} else {
								outbuffer[i] = memory[address+i-4+count];
							} 
							if (verbose) printf("%02X ", outbuffer[i]);
						}
						break;
					case 1: // overrule: JUMPV
						outbuffer[0] = 0x00;
						outbuffer[1] = 0x00;
						outbuffer[2] = (JUMP >> 8) & 0xFF;
						outbuffer[3] = (JUMP & 0xFF) | TYPE_VOID;
						if (verbose) printf("00 00 %02X %02X ", (JUMP >> 8) & 0xFF), (JUMP & 0xFF) | TYPE_VOID;
						read_overrule = 2;
						break;
					case 2: // overrule: 0
						outbuffer[0] = 0x00;
						outbuffer[1] = 0x00;
						outbuffer[2] = 0x00;
						outbuffer[3] = 0x00;
						if (verbose) printf("00 00 00 00 ");
						read_overrule = 0;
						break;
				}

				if (verbose) printf("\b]: ");

				/* print debug info */
				if (verbose) {
					if (action_text(&state, &substate, ACTION_READ, outbuffer, 
						message)) {

						printf(message);
					} else {
						printf("Unknown processor action");
					}
					if (confirm) {
						getchar();
					} else {
						printf("\n");
					}
				}
		
				/* write the data */
				rs232_write(serial, outbuffer, 4);
			}

			/* remove 5 bytes */
			updatebuffer(serial, &inbuffer, -5);
		} else {
			printf("Invalid command (2) [%02X]\n", 
				inbuffer.buffer[inbuffer.start+1]);
			synchronized = 0;
			/* remove 1 byte */
			updatebuffer(serial, &inbuffer, -1);
		}
	}
	
	/* destroy memory */
	free(memory);
	/* close the serial port */
	rs232_close(serial);
}

void printusage() {
	printf("Usage: x32-memserv -v -c comport [-s size] [-p progfile] \
[-f file location]...\n");
	printf("\n");
	printf("\tv:        verbose mode\n");
	printf("\ta:        confirmation mode (need -v)\n");
	printf("\n");
	printf("\tcomport:  serial port device (eg com1 or /dev/ttyS1)\n");
	printf("\tsize:     size of memory to emulate (KB)\n");
	printf("\tprogfile: filename to execute*\n");
	printf("\tlocation: location to load a file (in hex without prefix)**\n");
	printf("\tfile:     filename to load in memory**\n");
	printf("\n");
	printf("* The program file is loaded when the processor is finished \
booting, and is always stored at address 0. The first data read by the \
processor is overridden to contain a JUMPV instruction, the second \
to contain zero, thus the processor will automatically start executing \
the loaded program file at address 0.\n");
	printf("\n");
	printf("** Multiple files may be loaded by using the -f option multiple \
times. Note that the contents of the file may be overwritten by the \
processor when it's loading the code image from it's ROM.\n");
	printf("\n");
	printf("Example:\n");
	printf("\tx32-memserv -v -c /dev/ttyS1 -s 1024 -l 0:file1 1000:file2\n");
}

/* update input buffer with new bytes (diff>0 or remove old bytes (diff < 0) */
int updatebuffer(void* serial, BUFFER *buffer, int diff) {
	int newsize;
	int count, i;

	if (diff <= 0) {
		buffer->start -= diff;
		buffer->size += diff;
		newsize = buffer->size + diff;
	} else if (buffer->size < diff) {
		newsize = diff;
		if (buffer->size <= 0) {
			buffer->start = 0;
			buffer->size = 0;
		} else if (buffer->start + diff > 1024) {
			memcpy(&buffer[0], &buffer->buffer[buffer->start], buffer->size);
			buffer->start = 0;
		}

		while(buffer->size < newsize) {
			count = rs232_read(serial, &buffer->buffer[buffer->size+buffer->start], 1024-buffer->size-buffer->start);

			/* DEBUG: PRINTS ALL INCOMMING BYTES
			if (count > 0) {
				printf(" >> ");
				for (i = 0; i < count; i++) 
					printf("%02X ", buffer->buffer[buffer->size+buffer->start+i]);
				printf("\n");
			}
			*/	

			buffer->size += count;
		}
	}
}

/* load a file into the memory */
int load_file(char* filename, unsigned char* memory, int position, 
	int memory_size) {

	void* ptr;
	FILE* file;
	
	/* check file pointer */
	if (!(file = fopen(filename, "rb"))) return 1;
	
	/* start address */
	ptr = memory + position;
	
	/* check the size */
	fseek(file, 0, SEEK_END);
	if ((ftell(file) + position) > memory_size) {
		return 2;
	}

	/* copy the file */
	fseek(file, 0, SEEK_SET);
	while (!feof(file)) ptr += fread(ptr, 1, BUFF_SIZE, file);

	/* close file */
	fclose(file);

	return 0;
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
