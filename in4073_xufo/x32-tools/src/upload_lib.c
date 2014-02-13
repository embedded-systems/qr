#include "upload_lib.h"
#include <stdio.h>

void look_for_prompt();
 
/* rs232 send/receive functions */
void (*fpga_putchar)(char) = 0;
int (*fpga_getchar)() = 0;

int upload_file(char* filename, int location, int verbose, int fast) {
	char buffer[BUFFER_SIZE];
	char message[64];
	FILE* file;
	int total, size, i, bytes, c;

	/* open file */
	if ((file = fopen(filename, "rb")) == 0) return 0;

	/* get filesize */
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	if (verbose) printf("Connecting to X32 Loader application... ");
	if (verbose) fflush(stdout);
	
	/* send upload command */
	sprintf(message, "u %X %X b\0", location, location+size);
	if (!exec_command(message)) {
		if (verbose) printf("Could not start transfer\n");
		fclose(file);
		return 0;
	} else {
		if (verbose) printf("Connected\n");
		if (verbose) printf("Uploading %d KBytes... ", (size>>10)+1);
		if (verbose) fflush(stdout);
	}
	
	if (verbose) printf("    ");
	if (verbose) fflush(stdout);

	/* flush */
	while(fpga_getchar() != -1);

	/* start uploading */
	total = 0;
	while(!feof(file)) {
		bytes = fread(buffer, 1, BUFFER_SIZE, file);
		for (i = 0; i < bytes; i++) {
			fpga_putchar(buffer[i]);
		}
		if (!fast) {
			i = 0;
			while (i < bytes) {
				while((c = fpga_getchar()) == -1);
				
				if ((unsigned char)c != (unsigned char)buffer[i]) {
					if (i == 0 && total == 0) {
						/* allow first few chars to fail (junk left in buffer) */
						
						i--;
					} else {
						fclose(file);
						if (verbose) printf("\b\b\b\bTransfer failed [%d]!\n", total + i);
						if (verbose) fflush(stdout);
						return 0;
					}
				}
				i++;
			}
		}
		total += bytes;
		if (verbose) {
			if (total % 256 == 0) {
				printf("\b\b\b\b%3d%%", total*100/size);
				fflush(stdout);
			}
		}
	}
	
	if (verbose) printf("\b\b\b\bDone!\n");

	/* close file */
	fclose(file);

	/* flush buffer */
	while((c = fpga_getchar()) == -1);
	
	return 1;
}


int exec_command(char* command) {
	char* buff = command;
	char c;
	int i;

	/* wait for the prompt */
	look_for_prompt();
	
	/* assume the fpga's buffer size is large 
			enough to hold the entire command (or
			fast enough to keep up with rs232) */
	while(*buff) {
		fpga_putchar(*buff); 
		buff++;
	}
	/* check echo */
	i = 0;
	while(*command) {
		while((c = fpga_getchar()) == -1);
		/* allow first few characters to fail */
		if (c == *command) {
			i++;
			command++;
		} else if (i) {
			return 0;
		}
	}
	/* "finish" command */
	fpga_putchar('\n');

	while((c = fpga_getchar()) == -1);
	if (c == '\n') return 1;
	if (c != '\r') return 0;
	while((c = fpga_getchar()) == -1);
	if (c == '\n') return 1;

	return 1;
}


void look_for_prompt() {
	int c;

	/* clear buffer (make sure we get a fresh prompt) */
	while (c = fpga_getchar() >= 0);

	/* write newline (force new prompt) */
	fpga_putchar('\n');

	/* wait for prompt */
	while(1) {
		/* match '=' */
		while ((c = fpga_getchar()) != '=');
		/* match '>' */
		while ((c = fpga_getchar()) == -1);
		if (c == '>') {
			/* match ' ' */
			while ((c = fpga_getchar()) == -1);
			if (c == ' ') {
				return;
			}
		}
	}
}


void set_putchar(void (*rs232_putchar)(char)) {
	fpga_putchar = rs232_putchar;
}

void set_getchar(int (*rs232_getchar)()) {
	fpga_getchar = rs232_getchar;
}
