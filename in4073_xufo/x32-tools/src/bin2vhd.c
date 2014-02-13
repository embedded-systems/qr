#include <stdio.h>
#include <stdlib.h>
#include "bool.h"

void printUsage();

int main(int argc, char** argv) {
	FILE *sourcefile, *destinationfile;					/* source and destination file handle */
	unsigned char *data;
	int bytes, address;
	char* entity_name;
	
	int address_bus, data_bus, i;
	
	address_bus = 4;
	data_bus = 4;
	data = (unsigned char*)malloc(data_bus);
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

	fprintf(destinationfile, "library ieee;\n");
	fprintf(destinationfile, "use ieee.std_logic_1164.all;\n");
	fprintf(destinationfile, "use ieee.std_logic_arith.all;\n");
	fprintf(destinationfile, "use ieee.std_logic_unsigned.all;\n");
	fprintf(destinationfile, "\n");
	fprintf(destinationfile, "entity %s is\n", entity_name);
	fprintf(destinationfile, "\tport(\n");
	fprintf(destinationfile, "\t\tclk           : in  std_logic;\n");
	fprintf(destinationfile, "\t\taddress       : in  std_logic_vector(%d downto 0);\n", (address_bus<<3)-1);
	fprintf(destinationfile, "\t\tdata	        : out std_logic_vector(%d downto 0);\n", (data_bus<<3)-1);
	fprintf(destinationfile, "\t\tsize          : out std_logic_vector(%d downto 0)\n", (address_bus<<3)-1);
	fprintf(destinationfile, "\t);\n");
	fprintf(destinationfile, "end entity;\n");
	fprintf(destinationfile, "\n");
	fprintf(destinationfile, "architecture behaviour of %s is\n", entity_name);
	fprintf(destinationfile, "\tsignal data_i : std_logic_vector(%d downto 0);\n", (address_bus<<3)-1);
	fprintf(destinationfile, "begin\n");
	fprintf(destinationfile, "\tdata_i <=\n");
	
	address = 0;
	while(bytes = fread(data, 1, data_bus, sourcefile)) {
		fprintf(destinationfile, "\t\tx\"");
		for (i = 0; i < data_bus; i++) fprintf(destinationfile, "%02X", data[i]);
		fprintf(destinationfile, "\" when address = x\"");
		for (i = 0; i < address_bus; i++) fprintf(destinationfile, "%02X", (address >> ((address_bus - i-1) << 3)) & 0xFF);
		fprintf(destinationfile, "\" else\n");
		
		address += bytes;
	}
	
	address = address - address % data_bus;
	
	fprintf(destinationfile, "\t\t(others => '-');\n");
	fprintf(destinationfile, "\n");
	fprintf(destinationfile, "\tsize <= x\"");
	for (i = 0; i < address_bus; i++) fprintf(destinationfile, "%02X", ((address-bytes) >> ((address_bus - i-1) << 3)) & 0xFF);
	fprintf(destinationfile, "\";\n");
	fprintf(destinationfile, "\n");
	fprintf(destinationfile, "\tprocess(clk) begin\n");
	fprintf(destinationfile, "\t\tif (clk'event and clk = '1') then\n");
	fprintf(destinationfile, "\t\t\tdata <= data_i;\n");
	fprintf(destinationfile, "\t\tend if;\n");
	fprintf(destinationfile, "\tend process;\n");
	fprintf(destinationfile, "end architecture;\n");
	
	free(data);
	fclose(sourcefile);
	fclose(destinationfile);	
	
	
}

/*
 *	print usage to screen
 */
void printUsage() {
	printf("Usage:\n");
	printf("\tbin2vhd sourcefile -o destinationfile\n\n");
	printf("\tsourcefile:       source file location\n");
	printf("\tdestinationfile:  destination file location\n");
}
