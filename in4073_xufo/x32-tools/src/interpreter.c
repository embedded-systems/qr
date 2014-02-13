/* see interpreter.h for general information */
#include "interpreter.h"

typedef struct rf {
	char* filename;
	int position;
	struct rf *next;
} RAM_FILE;

/* 
 * application entry point
 */
int main(int argc, char** argv) {
	/* command line switches */
	BOOL is_trace_file, is_memory, no_discard, is_analysis_file, is_base_addr, 
		is_vid, is_extra_file;
	FILE* file;				/* filepointer for reading files */
	FILE* trace;			/* trace file */
	FILE* analysis;		/* analysis file */
	RAM_FILE *program_files;	/* program files */
	RAM_FILE *current;				/* pointer to last extra file */
	int i;
	Value* v;			/* main return value */
	/* interpreter state */
	Interpreter* interpreter;
	int vid;			/* version id */
	char* tmp; /* temporary string */
	
	/* interrupt variables */
	int interrupt_address, interrupt_priority;
	
	/* initialize */
	file = analysis = trace = 0;
	vid = 0;
	is_analysis_file = is_trace_file = is_memory = no_discard = is_base_addr = 
		is_vid = is_extra_file = FALSE;
	interpreter = create_interpreter(DEF_MEMORY_SIZE);

	if (!init_peripherals()) {
		printf("Peripheral initialization failed!\n");
		exit(-1);
	}

	/* randomizer */
	srand((unsigned)time(NULL));
	
	current = program_files = malloc(sizeof(RAM_FILE));
	current->next = 0;
	current->filename = 0;
	current->position = 0;

	/* parse command line */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			/* switch */
			if (strcmp(&argv[i][1], "t") == 0) {
				is_trace_file = TRUE;
			} else if (strcmp(&argv[i][1], "m") == 0) {
				is_memory = TRUE;
			} else if (strcmp(&argv[i][1], "nd") == 0) {
				no_discard = TRUE;
			} else if (strcmp(&argv[i][1], "a") == 0) {
				is_analysis_file = TRUE;
			} else if (strcmp(&argv[i][1], "f") == 0) {
				is_extra_file = TRUE;
			} else if (strcmp(&argv[i][1], "base") == 0) {
				is_base_addr = TRUE;
			} else if (strcmp(&argv[i][1], "vid") == 0) {
				is_vid = TRUE;
			} else {
				printf("Invalid switch %s\n", argv[i]);
				printUsage();
				term_peripherals();
				exit(-1);
			}
		} else if (is_memory) {
			/* set memory size */
			interpreter->memory_size = atoi(argv[i]);
		} else if (is_trace_file) {
			/* load trace file */
			if (!trace) {
				trace = fopen(argv[i], "w");
			} else {
				printf("Multiple trace files specified\n");
				printUsage();
				term_peripherals();
				exit(-1);
			}
			is_trace_file = FALSE;
		} else if (is_analysis_file) {
			/* load trace file */
			if (!analysis) {
				analysis = fopen(argv[i], "w");
			} else {
				printf("Multiple analysis files specified\n");
				printUsage();
				term_peripherals();
				exit(-1);
			}
			is_analysis_file = FALSE;
		} else if (is_base_addr) {
			/* base address */
			program_files->position = hex2bin(argv[i]);
			if (program_files->position < 0) {
				printf("Invalid base address, must be hexadecimal\n");
				program_files->position = 0;
			}
			is_base_addr = FALSE;
		} else if (is_vid) {
			/* version id */
			vid = hex2bin(argv[i]);
			if (vid < 0) {
				printf("Invalid version id, must be hexadecimal\n");
				vid = 0;
			}
			is_vid = FALSE;
		} else if (is_extra_file) {
			/* extra file in memory */
			current = (current->next = malloc(sizeof(RAM_FILE)));
			current->next = 0;
			current->filename = malloc(strlen(argv[i])+1);
			strcpy(current->filename, argv[i]);
			if (tmp = strrchr(current->filename, ':')) {
				current->position = hex2bin(tmp+1);
				*tmp = '\0';
			} else {
				current->position = 0;
			}
		
			is_extra_file = FALSE;
		} else {
			/* load executable */
			if (program_files->filename == 0) {
				program_files->filename = malloc(strlen(argv[i])+1);
				strcpy(program_files->filename, argv[i]);
			} else {
				printf("Multiple source files specified\n");
				printUsage();
				term_peripherals();
				exit(-1);
			}
		}
	}
	
	if (!vid) vid = DEF_VID;
	
	if (is_trace_file) {
		/* -t is given, but no filename */
		printf("No trace or analysis file specified, remove -t or -a flag from commandline\n");
		printUsage();
		term_peripherals();
		exit(-1);
	}
	if (!program_files->filename) {
		/* no source file given */
		printf("No source file specified %x\n", program_files->filename);
		printUsage();
		term_peripherals();
		exit(-1);
	}
	
	/* load all extra files */
	current = program_files->next;
	while(current) {
		printf("Loading %s to address 0x%08X\n", current->filename, current->position);
		file = fopen(current->filename, "rb");
		if (file) {
			load_program(file, interpreter, current->position);
			fclose(file);
		} else {
			printf("Could not open %s, file not loaded\n", current->filename);
		}
		current = current->next;
	}

	/* load program file */
	current = program_files;
	printf("Loading %s to address 0x%08X\n", current->filename, current->position);
	file = fopen(current->filename, "rb");
	if (file) {
		load_program(file, interpreter, current->position);
		fclose(file);
	} else {
		printf("Could not open %s, file not loaded\n", current->filename);
		printUsage();
		term_peripherals();
		exit(-1);
	}

	/* extra options */
	interpreter->allow_discard = !no_discard;
	interpreter->pc = program_files->position;
	interpreter->vid = vid;
	
	printf("Running program:\n");
	printf("\tMemory size: %d bytes\n", interpreter->memory_size);
	printf("\tProgram size: %d bytes\n", interpreter->program_size);
	printf("\tRoom left for stack: %d bytes (%d words)\n", interpreter->memory_size -
		interpreter->program_size, (interpreter->memory_size - interpreter->program_size)/INT_SIZE);
	printf("\n\n");

	/* execution loop */
	int error;
	error = ERR_NOERROR;
	while(error == ERR_NOERROR) {
		if(trace) dumpstate(interpreter, trace);
		
		/* check for interrupts */
		if (interrupt_pending(&interrupt_address, &interrupt_priority, 
			interpreter->el, interpreter->trapped, interpreter->overflow,
			interpreter->div0, interpreter->out_of_memory)) {

			error = interrupt(interpreter, interrupt_address, interrupt_priority);
		} else {
			error = execute(interpreter);
		}

		if(trace) dumpstack(interpreter, trace, 0xFF);
	}
	
	if(trace) fflush(trace);
	
	/* check exit code */
	if (error != ERR_EXIT_PROGRAM && error != ERR_TRAPPED) {
		printf("Error code: %d (%s)\n", error, errmsg(error));
		printf("\tAfter %d instructions\n", interpreter->executed);
		/* dump state to screen */
		dumpstate(interpreter, stdout);
		/* dump stack to screen */
		dumpstack(interpreter, stdout, 0xFF);
	} else {
		/* get return value from stack */
		v = create_int(0);
		pop_value(interpreter, v);
		i = parse_int(v);
		/* print some info */
		printf("\n\nProgram return value: %d (0x%08X)\n", i, i);
		printf("Halt instruction at: 0x%08X\n", interpreter->pc);
		printf("%d instructions executed\n", interpreter->executed);
		printf("%d bytes peak memory usage (%d words)\n", interpreter->stackpeak,
					 interpreter->stackpeak/INT_SIZE);
		
		if (analysis && interpreter->executed > 0) {
			fprintf(analysis, "Total number of instructions executed: %d\n", interpreter->executed);
			fprintf(analysis, "Bytes used by dynamic memory: %d\n", interpreter->stackpeak);
			fprintf(analysis, "Aligned memory actions: %d (%2.2f%%)\n", interpreter->aligned,
				100*(double)interpreter->aligned/(interpreter->aligned+interpreter->unaligned));
			fprintf(analysis, "Unaligned memory actions: %d (%2.2f%%)\n", interpreter->unaligned,
				100*(double)interpreter->unaligned/(interpreter->aligned+interpreter->unaligned));
			fprintf(analysis, "\n");

			for (i = 0; i < MAX_OPCODE_ID; i++) {
				if (opcode_from_id(i) != 0) {
					fprintf(analysis, "%s:\t%d\t%2.2f%%\n", opcode_name(opcode_from_id(i)),
									interpreter->analysis[i],
									100*(double)interpreter->analysis[i]/(double)interpreter->executed);
				}
			}
			fclose(analysis);
		}
		
	}
	
	/* cleanup */
	if (trace) fclose(trace);
	destroy_interpreter(interpreter);
	
	term_peripherals();
	
	return 0;
}   

/*
 * decode error code to string
 */
char* errmsg(int errcode) {
	switch(errcode) {
		case ERR_NOERROR: return "No error occured";
		case ERR_INVALID_OPCODE: return "Invalid opcode";
		case ERR_INVALID_TYPE: return "Invalid instruction type or size";
		case ERR_UNIMPLEMENTED: return "Instruction unimplemented";
		case ERR_OUT_OF_STACK: return "Out of stack space";
		case ERR_MEMREAD_ERROR: return "Can't read from memory";
		case ERR_MEMWRITE_ERROR: return "Can't write to memory";
		case ERR_PC_ERROR: return "Invalid program counter";
		case ERR_TRAPPED: return "Breakpoint not handled";
		default: return "Unknown error";
	}
}


/*
 * print usage to screen
 */
void printUsage() {
	printf("Usage:\n");
	printf("\tinterpreter inputfile [-m memorysize] [-t tracefile] [-a analysis file]\n\n");
}

/*
 * print error to screen, dump stack and interpreteressor state
 */
void error(Interpreter* interpreter, char* str) {
	dumpstate(interpreter, stderr);
	dumpstack(interpreter, stderr, 0xFF);
	fprintf(stderr, "ERROR: %s\n", str);
	term_peripherals();
	exit(-1);
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
