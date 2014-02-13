#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bool.h"

void printUsage();

void binary_to_bit(unsigned char byte, unsigned char *bit_string) {
	
	unsigned char z;
 	int i;

	z = 128;

	for ( i = 0; i < 8; i++){ 
		bit_string[i] =  ((byte & z) == z) ? '1' : '0';
		z >>= 1;
	}

	bit_string[8] = 0;
}

int main(int argc, char** argv) {
	FILE *sourcefile, *destinationfile;					/* source and destination file handle */
	unsigned char *data, *bit_data;
	int bytes, address,memory_size;
	char* entity_name;
	
	int address_bus, data_bus, i;
	
	address_bus = 4;
	data_bus = 4;
	data = (unsigned char*)malloc(data_bus);
	bit_data = (unsigned char *)malloc(9);
	memory_size = 0x10000;
	entity_name = "rom";
	
	/* flags */
	BOOL is_output_file;
	/* init variables */
	sourcefile = destinationfile = 0;
	is_output_file = FALSE;
	


	/* parse the command line */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			/* switch */
			if (strcmp(&argv[i][1], "o") == 0) {
				is_output_file = TRUE;
			} else {
				printf("Invalid switch %s", argv[i]);
				printUsage();
				exit(-1);
			}
		} else if (is_output_file) {
			/* output file name */
			if (destinationfile == 0) {
				destinationfile = fopen(argv[i], "w");
				if (destinationfile == 0) {
					printf("Could not create output file %s.", argv[i]);
					exit(-1);
				}
				is_output_file = FALSE;
			} else {
				printf("Multiple output files given\n");
				printUsage();
				exit(-1);
			}
		} else {
			/* input file name */
			if (sourcefile == 0) {
				sourcefile = fopen(argv[i], "rb");
				if (sourcefile == 0) {
					printf("Source file %s not found.", argv[i]);
					exit(-1);
				}
			} else {
				printf("Multiple source files given\n");
				printUsage();
				exit(-1);
			}
		}
		 
	}
	
	if (destinationfile == 0 || sourcefile == 0) {
		printUsage();
		exit(EXIT_FAILURE);
	}

	
	address = 0;
	while( (bytes = fread(data, 1, data_bus, sourcefile)) ) {
			
		for (i = 0; i < data_bus; i++){
			binary_to_bit(data[i], bit_data);
			fprintf(destinationfile, "%s", bit_data);
			//printf("%s", bit_data);
		
		}
		fprintf(destinationfile, "\n");
		address += bytes;
	}


	binary_to_bit(0, bit_data);
	while ( address < memory_size) {
		
		fprintf(destinationfile, "%s%s%s%s\n", bit_data, bit_data, bit_data, bit_data);
		address += 4;
	}		


	
	free(data);
	free(bit_data);
	fclose(sourcefile);
	fclose(destinationfile);	
	
	return (0);
}

/*
 *	print usage to screen
 */
void printUsage() {
	printf("Usage:\n");
	printf("\tbin2bit sourcefile -o destinationfile\n\n");
	printf("\tsourcefile:       source file location\n");
	printf("\tdestinationfile:  destination file location\n");
}
