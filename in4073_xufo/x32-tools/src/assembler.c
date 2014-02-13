/* see assembler.h for general information */

#include "assembler.h"
#include "instructions.h"
#include "bool.h"
#include "x32_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int linenr;		/* current source file line number */
int hex2bin(char*);

/*
 *	Application entry point
 */
int main(int argc, char **argv) {
	FILE *file;										/* source and destination file handle */
	char* sourcefile;							/* source file name */
	char* targetfile;							/* destination file name */
	Assembly* assembly;						/* assembly structure */
	Label *label,*reflabel;				/* temp label structure */
	int i, labels;								/* loop integer and label counter */
	HashTableEnumeration* henum;	/* temp enumeration value */
	/* processor version id */
	int proc_version_id;
	
	/* flags */
	BOOL is_output_file, debug, is_version_id;
	/* init variables */
	sourcefile = targetfile = 0;
	is_output_file = debug = is_version_id = FALSE;
	proc_version_id = 0;
	
	/* parse the command line */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			/* switch */
			if (strcmp(&argv[i][1], "o") == 0) {
				is_output_file = TRUE;
			} else if (strcmp(&argv[i][1], "g") == 0) {
				debug = TRUE;
			} else if (strcmp(&argv[i][1], "vid") == 0) {
				is_version_id = TRUE;
			} else {
				printf("Invalid switch %s\n", argv[i]);
				printUsage();
				exit(-1);
			}
		} else if (is_output_file) {
			/* output file name */
			if (targetfile == 0) {
				targetfile = argv[i];
				is_output_file = FALSE;
			} else {
				printf("Multiple output files given\n");
				printUsage();
				exit(-1);
			}
		} else if (is_version_id) {
			/* version id */
			if (proc_version_id == 0) {
				proc_version_id = hex2bin(argv[i]);
				is_version_id = FALSE;
			} else {
				printf("Multiple version id's given\n");
				printUsage();
				exit(-1);
			}
		} else {
			/* input file name */
			if (sourcefile == 0) {
				sourcefile = argv[i];
			} else {
				printf("Multiple source files given\n");
				printUsage();
				exit(-1);
			}
		}
		 
	}

	if (proc_version_id == 0x0000) proc_version_id = DEF_PROC_VERSION_ID;
	
	/* open the source file */
	file = fopen(sourcefile, "rb");
	if (file == 0) {
		printf("Could not open source file '%s'\n", sourcefile);
		printUsage();
		exit(EXIT_FAILURE);
	}
	/* parse file */
	assembly = parse_file(file);
	fclose(file);
	
	/*
	printLabels(assembly);
	*/
	
	/* write output */
	file = fopen(targetfile, "wb");
	if (file == 0) {
		printf("Could not open target file");
		exit(-1);
	}
	
	/* magic marker */
	fputc('S', file); fputc('O', file); fputc('B', file); fputc('J', file);
	/* make room for nr of labels (sizeof(int)) */
	for (i = 0; i < sizeof(int); i++) fputc('\0', file);

	/* start enumeration over the labels */
	henum = HT_StartEnumeration(assembly->labels);
	labels = 0;	/* label counter */

	while(label = (Label*)HT_GetNextItem(henum)) {
		/* dereference */
		reflabel = label;
		while(reflabel->ref) reflabel = reflabel->ref;
		if (reflabel != label) {
			label->location = reflabel->location;
			label->segment = reflabel->segment;
		}

		/* do not write nonaccessed imported labels */
		if (!label->imported || label->accessed) {
			/* write label to file */
			/* name length */
			fputc(strlen(label->name), file);
			/* name */
			fputs(label->name, file);
			/* imported/exported flags */
			if (label->exported) {
				fputc(1, file);
			} else if (label->imported) {
				fputc(2, file);
			} else if (label->debuginfo != 0) {
				fputc(1+(label->debuginfo<<4), file);
			} else {
				fputc(0, file);
			} 
			/* segment */
			fputc((char)label->segment, file);
			/* id */
			fwrite(&label->id, sizeof(int), 1, file);
			/* location */
			fwrite(&label->location, sizeof(int), 1, file);
			/* update counter */
			labels++;
		}
	}
	/* stop the enumeration */
	HT_StopEnumeration(henum);
	
	/* align the segments to 8, (to "finish" the bitmaps) */
	align_segment(assembly->code, 8);
	align_segment(assembly->lit, 8);
	align_segment(assembly->data, 8);
	
	/* write the binary segment data and the label masks */
	/* size */
	fwrite(&assembly->code->data->size, sizeof(int), 1, file);
	/* data */
	MS_Dump(assembly->code->data, file);
	/* mask */
	MS_Dump(assembly->code->mask, file);
	/* size */
	fwrite(&assembly->lit->data->size, sizeof(int), 1, file);
	/* data */
	MS_Dump(assembly->lit->data, file);
	/* mask */
	MS_Dump(assembly->lit->mask, file);
	/* size */
	fwrite(&assembly->data->data->size, sizeof(int), 1, file);
	/* data */
	MS_Dump(assembly->data->data, file);
	/* mask */
	MS_Dump(assembly->data->mask, file);
	/* sizeof bss segment, segment itself contains uninitialized data, doesn't
			need to be stored in assembly file */
	fwrite(&assembly->bss_size, sizeof(int), 1, file);
	/* write label count */
	fseek(file, 4, SEEK_SET);
	fwrite(&labels, sizeof(int), 1, file);
	
	/* close output file */
	fclose(file);
	return 0;
}

/*
 *	return segment name
 */
char* get_segment_name(int segment) {
	switch(segment) {
		case UNKNOWN: 	return "UNKNOWN";
		case CODE: 			return "CODE";
		case DATA: 			return "DATA";
		case LIT: 			return "LIT";
		case BSS: 			return "BSS";
	}
} 

/*
 *	print usage to screen
 */
void printUsage() {
	printf("Usage:\n");
	printf("\tassembler sourcefile -o destinationfile\n\n");
	printf("\tsourcefile:       source file location\n");
	printf("\tdestinationfile:  destination file location\n");
}


/*
 *	print all labels in assembly to screen (debug)
 */
void printLabels(Assembly* assembly) {
	HashTableEnumeration* henum;
	Label* label;

	/* start enumeration over labels */
	henum = HT_StartEnumeration(assembly->labels);

	printf("Labels:\n");
	while(label = (Label*)HT_GetNextItem(henum)) {
		/* print label properties */
		printf("%-20s%-4d%-10s  0x%08X           %-3s   (%d)\n", label->name, label->id, get_segment_name(label->segment), label->location, label->exported ? "" : label->imported ? "E" : "L", label->accessed);
	}
	/* stop enumeration */
	HT_StopEnumeration(henum);
}

/*
 *	assemble a file and load into memory
 */
Assembly* parse_file(FILE* file) {
	Assembly* assembly;					/* assembly structure */
	int labelid;								/* new label id */
	Segment* current_segment;		/* holds current segment */
	char line[MAX_LINE];				/* holds one line */
	char* items[MAX_ARGS];			/* split line into items (split on ' ' and '\t') */
	int segment;								/* holds current segment id */
	int count, i, size;					/* loop variables */
	long long params[MAX_ARGS];	/* parsed items array */
	char sbuffer[1024];					/* 1K string buffer */
	char* sourcefile;						/* current source file */
	Label* label;
	
	/* initialize variables */
	assembly = new_assembly();
	labelid = 0;
	
	linenr = 0;
	segment = UNKNOWN;
	sourcefile = NULL;
	
	/* read a line lines (returns nr of characters read, -1 if eof) */
	while(read_line(file, line, MAX_LINE) >= 0) {
		linenr++;
		/* get rid of spaces/tabs in front of the line */
		trim_line(line);

		/* split the line on spaces/tabs */
		count = split_line(line, items);
		/* note: line == items[0] */
		
		/* check if the line was not empty */
		if (strlen(line) > 0) {
			if (strcmp(line, "code") == 0) {
				/* code segment */
				segment = CODE;
				current_segment = assembly->code;
			} else if (strcmp(line, "bss") == 0) {
				/* bss segment */
				segment = BSS;
				/* bss has no segment structure */
				/* (no point in saving uninitialized data */
				current_segment = 0;
			} else if (strcmp(line, "lit") == 0) {
				/* lit segment */
				segment = LIT;
				current_segment = assembly->lit;
			} else if (strcmp(line, "data") == 0) {
				/* data segment */
				segment = DATA;
				current_segment = assembly->data;
			} else if (strcmp(line, "export") == 0) {
				/* mark label as public */
				if (count != 2) error(linenr, "invalid number of parameters");
				label = get_label(assembly, items[1]);
				/* any exported function should be included */
				label->accessed = 1;
				label->exported = TRUE;
			} else if (strcmp(line, "import") == 0) {
				/* mark label as imported */
				if (count != 2) error(linenr, "invalid number of parameters");
				get_label(assembly, items[1])->imported = TRUE;
			} else if (segment == UNKNOWN) {
				/* all other things must be in segments */
				error(linenr, "code outside segment");
			} else if (strcmp(line, "align") == 0) {
				/* align a segment */
				if (count != 2) error(linenr, "invalid number of parameters");
				params[0] = parse_numeric(items[1]);
				if (segment == BSS) {
					/* align bss just by size */
					while(assembly->bss_size % params[0]) assembly->bss_size++;
				} else {
					align_segment(current_segment, params[0]);
				}
			}	else if (strcmp(line, "byte") == 0) {
				/* one byte of data */
				if (count != 3) error(linenr, "invalid number of parameters");
				if (segment == BSS) {
					error(linenr, "can't put initialized data in bss, use data segment instead");
				} else {
					/* parse */
					params[0] = parse_numeric(items[1]);
					params[1] = parse_numeric(items[2]);
					/* write to segment */
					MS_WriteBE(current_segment->data, params[1], (int)params[0]);
					for (i = 0; i < (int)params[0]; i++) MS_WriteBit(current_segment->mask, FALSE);
				}
			}	else if (strcmp(line, "skip") == 0) {
				/* some empty space */
				if (count != 2) error(linenr, "invalid number of parameters");
				/* parse */
				params[0] = parse_numeric(items[1]);
				if (segment == BSS) {
					/* for bss: just update the size */
					assembly->bss_size += params[0];
				} else {
					/* for other segment: write zeros */
					MS_Write(current_segment->data, 0, (int)params[0]);
					for (i = 0; i < (int)params[0]; i++) MS_WriteBit(current_segment->mask, FALSE);
				}
			} else if (strcmp(line, "address") == 0) {
				/* insert address to label here */

				/* some empty space */
				if (count != 2) error(linenr, "invalid number of parameters");
				/* write the id to the current segment */
				MS_WriteBE(current_segment->data, get_label(assembly, items[1])->id, PTR_SIZE);
				/* needs to be resolved */
				for (i = 0; i < PTR_SIZE; i++) MS_WriteBit(current_segment->mask, TRUE);
			} else if (strcmp(line, "line") == 0) {
				if (count < 2) error(linenr, "invalid number of parameters");
				if (sourcefile == NULL) error(linenr, "file directive must precede line directive");

				size = sprintf(sbuffer, "$%s:", sourcefile);
				for (i = 1; i < count; i++) {
					size += sprintf(sbuffer+size, "%s", items[i]);
					if (i == count-1) {
						sbuffer[size++] = 0;
					} else {
						sbuffer[size++] = ' ';
					}
				}
				
				/* register debug label */
				label = get_label(assembly, sbuffer);
				if (label->segment == UNKNOWN) {
					if (segment == BSS) {
						label->location = assembly->bss_size;
					} else {
						label->location = current_segment->data->size;
					}
					label->segment = segment;
				}
				/* mark as debug function, set accessed to 1 to include it in the assembly */
				label->accessed = 1;
				label->debuginfo = LABEL_DEBUG_LINE;
			} else if (strcmp(line, "file") == 0) {
				if (count < 2) error(linenr, "invalid number of parameters");

				size = sprintf(sbuffer, "$");
				for (i = 1; i < count; i++) {
					size += sprintf(sbuffer+size, "%s", items[i]);
					if (i == count-1) {
						sbuffer[size++] = 0;
					} else {
						sbuffer[size++] = ' ';
					}
				}

				if (sourcefile != NULL) free(sourcefile);
				sourcefile = malloc(size-1);
				strcpy(sourcefile, sbuffer+1);
				
				/* register debug label */
				label = get_label(assembly, sbuffer);
				if (segment == BSS) {
					label->location = assembly->bss_size;
				} else {
					label->location = current_segment->data->size;
				}
				label->segment = segment;
				/* mark as debug function, set accessed to 1 to include it in the assembly */
				label->accessed = 1;
				label->debuginfo = LABEL_DEBUG_FILE;
			} else if (strcmp(line, "local") == 0) {
				if (count < 3) error(linenr, "invalid number of parameters");

				size = sprintf(sbuffer, "$");
				for (i = 1; i < count; i++) {
					if (i != 2) {
						size += sprintf(sbuffer+size, "%s", items[i]);
						if (i == count-1) {
							sbuffer[size++] = 0;
						} else {
							sbuffer[size++] = ' ';
						}
					}
				}
				
				/* register debug label */
				label = get_label(assembly, sbuffer);
				label->location = parse_numeric(items[2]);				

				label->segment = UNKNOWN;
				/* mark as debug function, set accessed to 1 to include it in the assembly */
				label->accessed = 1;
				label->debuginfo = LABEL_DEBUG_LOCAL;
			} else if (strcmp(line, "param") == 0) {
				if (count < 3) error(linenr, "invalid number of parameters");

				size = sprintf(sbuffer, "$");
				for (i = 1; i < count; i++) {
					if (i != 2) {
						size += sprintf(sbuffer+size, "%s", items[i]);
						if (i == count-1) {
							sbuffer[size++] = 0;
						} else {
							sbuffer[size++] = ' ';
						}
					}
				}
				
				/* register debug label */
				label = get_label(assembly, sbuffer);
				label->location = parse_numeric(items[2]);
				label->segment = UNKNOWN;
				/* mark as debug function, set accessed to 1 to include it in the assembly */
				label->accessed = 1;
				label->debuginfo = LABEL_DEBUG_PARAM;
			} else if (strcmp(line, "global") == 0) {
				if (count < 2) error(linenr, "invalid number of parameters");

				size = sprintf(sbuffer, "$");
				for (i = 1; i < count; i++) {
					size += sprintf(sbuffer+size, "%s", items[i]);
					if (i == count-1) {
						sbuffer[size++] = 0;
					} else {
						sbuffer[size++] = ' ';
					}
				}
				
				/* register debug label */
				label = get_label(assembly, sbuffer);
				label->ref = get_label(assembly, items[1]);
				/* mark as debug function, set accessed to 1 to include it in the assembly */
				label->accessed = 1;
				label->debuginfo = LABEL_DEBUG_GLOBAL;
			} else if (strcmp(line, "function") == 0) {
				if (count < 2) error(linenr, "invalid number of parameters");

				size = sprintf(sbuffer, "$");
				for (i = 1; i < count; i++) {
					size += sprintf(sbuffer+size, "%s", items[i]);
					if (i == count-1) {
						sbuffer[size++] = 0;
					} else {
						sbuffer[size++] = ' ';
					}
				}
				
				/* register debug label */
				label = get_label(assembly, sbuffer);
				label->ref = get_label(assembly, items[1]);
				/* mark as debug function, set accessed to 1 to include it in the assembly */
				label->accessed = 1;
				label->debuginfo = LABEL_DEBUG_FUNCTION;
			} else if (strcmp(line, "label") == 0) {
				/* a label; register */
				if (count != 2) error(linenr, "invalid number of parameters");

				if (segment == BSS) {
					register_label(assembly, items[1], segment, assembly->bss_size);
				} else {
					register_label(assembly, items[1], segment, current_segment->data->size);
				}
			} else if (strcmp(line, "typedef") == 0) {
				if (count < 2) error(linenr, "invalid number of parameters");

				size = sprintf(sbuffer, "$");
				for (i = 1; i < count; i++) {
					size += sprintf(sbuffer+size, "%s", items[i]);
					if (i == count-1) {
						sbuffer[size++] = 0;
					} else {
						sbuffer[size++] = ' ';
					}
				}

				/* register debug label */
				label = get_label(assembly, sbuffer);
				/* mark as debug function, set accessed to 1 to include it in the assembly */
				label->accessed = 1;
				label->debuginfo = LABEL_DEBUG_TYPEDEF;
			} else if (strcmp(line, "field") == 0) {
				if (count < 2) error(linenr, "invalid number of parameters");

				size = sprintf(sbuffer, "$");
				for (i = 1; i < count; i++) {
					if (i != 2) {
						size += sprintf(sbuffer+size, "%s", items[i]);
						if (i == count-1) {
							sbuffer[size++] = 0;
						} else {
							sbuffer[size++] = ' ';
						}
					}
				}

				/* register debug label */
				label = get_label(assembly, sbuffer);
				label->location = parse_numeric(items[2]);				

				label->segment = UNKNOWN;
				/* mark as debug function, set accessed to 1 to include it in the assembly */
				label->accessed = 1;
				label->debuginfo = LABEL_DEBUG_FIELD;
			} else {
				/* not identified; should be an instruction */
				write_instruction(assembly, items, count, current_segment);
			}
		}
	}	
	
	if (sourcefile != NULL) free(sourcefile);

	/* return the structure */
	return assembly;
}

/*
 * aligns a segment (also updates label mask 
 */
void align_segment(Segment* segment, int alignment) {
	int size, i;

	/* align memory stream, check number of bytes added */
	size = segment->data->size;
	MS_Align(segment->data, alignment);
	size = segment->data->size - size;
	/* add same number of bits to label mask */
	for (i = 0; i < size; i++) MS_WriteBit(segment->mask, FALSE);
}

/*
 * find a label given it's id
 */
Label* get_label_from_id(Assembly* assembly, int id) {
	HashTableEnumeration* henum;
	Label* label;
	
	/* enumerate the labels */
	henum = HT_StartEnumeration(assembly->labels);

	while(label = (Label*)HT_GetNextItem(henum)) {
		/* compare id */
		if (label->id == id) {
			HT_StopEnumeration(henum);
			return label;
		}
	}
	/* not found */
	HT_StopEnumeration(henum);
	return 0;
}

/*
 *	find a label, given it's name. if the label is not
 *		found, it is added to the label table
 */
Label* get_label(Assembly* assembly, char* name) {
	static int id = 0;
	Label* label;
	
	/* find it */
	label = (Label*)HT_Find(assembly->labels, name);
	
	if (!label) {
		/* not found, create */
		label = (Label*)malloc(sizeof(Label));
		label->name = malloc(strlen(name)+1);
		strcpy(label->name, name);
	
		label->segment = UNKNOWN;
		label->location = 0;
		label->exported = FALSE;
		label->imported = FALSE;
		label->debuginfo = LABEL_DEBUG_NONE;
		label->accessed = 0;
		label->id = ++id;
		label->ref = 0;
		/* add */
		HT_Add(assembly->labels, label, name);
	} else {
		label->accessed++;
	}
	/* return */
	return label;
}

/*
 * set the location of a label 
 */
Label* register_label(Assembly* assembly, char* name, int segment, int location) {
	Label* label;
	
	/* find/create the label */
	label = get_label(assembly, name);
	if (label->segment != UNKNOWN) {
		/* location allready set => multiple labels with the same name */
		printf("Label '%s' points to multiple locations\n", name);
		error(linenr, "Duplicate label found");
	} else {
		/* set location and segment */
		label->segment = segment;
		label->location = location;
		return label;
	}
}

/*
 * create a new empty assembly
 */
Assembly* new_assembly() {
	Assembly* assembly;
	
	/* allocate structuere memory */
	assembly = malloc(sizeof(Assembly));
	/* create all segments */
	assembly->code = malloc(sizeof(Segment));
	assembly->code->data = MS_Open(DEFCODESIZE);
	assembly->code->mask = MS_Open(DEFCODESIZE>>3);
	assembly->code->segment = CODE;
	assembly->lit = malloc(sizeof(Segment));
	assembly->lit->data = MS_Open(DEFLITSIZE);
	assembly->lit->mask = MS_Open(DEFLITSIZE>>3);
	assembly->lit->segment = LIT;
	assembly->data = malloc(sizeof(Segment));
	assembly->data->data = MS_Open(DEFDATASIZE);
	assembly->data->mask = MS_Open(DEFDATASIZE>>3);
	assembly->data->segment = DATA;
	assembly->bss_size = 0;
	assembly->labels = HT_Create();
	
	/* return */
	return assembly;
}

/*
 * assembles an instruction and write the binary code to the segment 
 */
BOOL write_instruction(Assembly* assembly, char* params[], int count, Segment* segment) {
	int type, size, opcode, i;	/* instruction parts */
	Value value;								/* instruction parameter */
	unsigned char* instruction;	/* binary instruction */
	int paramtype;							/* parameter type */

	if (parse_instruction(params[0], &opcode, &size, &type)) {
		switch(opcode) {
			case ADDRF: case ADDRL: case ADDRA:
				/* integer (sizeof pointer) parameter in file */
				if (count != 2) error(linenr, "invalid number of parameters (need 1)");
				paramtype = PARAM_INT;
				value = parse_value(params[1], INTEGER, PTR_SIZE);
				break;
			case CVF: 
				error(linenr, "floating point conversion not supported");
			case CVI: case CVP: case CVU:
				/* integer parameter in file */
				if (count != 2) error(linenr, "invalid number of parameters (need 1)");
				paramtype = PARAM_NONE;
				value = parse_value(params[1], INTEGER, 1);
				
				/* printf("INSTRUCTION: %s %s:, SIZE: %d, PARAMETER: %d\n", params[0], params[1], size, value.u); */
				
				/* build last opcode byte, use value.u as temp variable */
				if (value.u == 1) {
					value.u = TYPE_CHAR + (opcode == CVI?1:0);
				} else if (value.u == INT_SIZE) {
					value.u = TYPE_INT + (opcode == CVI?1:0);
				} else if (value.u == SHRT_SIZE) {
					value.u = TYPE_SHORT + (opcode == CVI?1:0);
				} else if (value.u == LNG_SIZE) {
					value.u = TYPE_LONG + (opcode == CVI?1:0);
				} else if (value.u == PTR_SIZE) {
					value.u = TYPE_POINTER + (opcode == CVI?1:0);
				} else {
					error(linenr, "invalid conversion");
				}
				if (size == 1) {
					value.u |= ((TYPE_CHAR + (type == INTEGER?1:0))<<4);
				} else if (size == INT_SIZE) {
					value.u |= ((TYPE_INT + (type == INTEGER?1:0))<<4);
				} else if (size == SHRT_SIZE) {
					value.u |= ((TYPE_SHORT + (type == INTEGER?1:0))<<4);
				} else if (size == LNG_SIZE) {
					value.u |= ((TYPE_LONG + (type == INTEGER?1:0))<<4);
				} else if (size == PTR_SIZE) {
					value.u |= ((TYPE_POINTER + (type == INTEGER?1:0))<<4);
				} else {
					error(linenr, "invalid conversion");
				}

				/* printf("OPCODE: 0x%04X, VALUE: 0x%04X, COMBINED: ", opcode, value); */
				opcode = (opcode & 0xFF00) | (value.u & 0xFF);
				/* printf("0x%04X\n", opcode); */

				break;
			case MOVESTACK:
				/* pseudo instruction for VARSTACK */
				//opcode = VARSTACK;
				if (count != 2) error(linenr, "invalid number of parameters (need 1)");
				value.i = get_label(assembly, params[1])->id;
				paramtype = PARAM_POINTER;
				break;				
			case ARGSTACK: case VARSTACK: case ARG:
				/* integer parameter in file */
				if (count != 2) error(linenr, "invalid number of parameters (need 1)");
				paramtype = PARAM_INT;
				value = parse_value(params[1], INTEGER, INT_SIZE);
				break;
			case ADDRG: case EQ: case GE: case GT: case LE: case LT: case NE:
				/* pointer parameter in file */
				if (count != 2) error(linenr, "invalid number of parameters (need 1)");
				value.i = get_label(assembly, params[1])->id;
				paramtype = PARAM_POINTER;
				break;
			case CNST:
				/* constant parameter in file */
				if (count != 2) error(linenr, "invalid number of parameters (need 1)");
				paramtype = PARAM_CONST;
				value = parse_value(params[1], type, size);
				break;
			case SAVELP: case SAVEFP: case SAVEAP: case SAVESP: case SAVEVR: case SAVEEL:
				/* pseudo instructions! */
				if (count != 1) error(linenr, "invalid number of parameters (need 0)");
				opcode = (opcode & 0x00FF) | 0x0100;
				paramtype = PARAM_INT;
				value.i = 0;
				break;
			case BCOM: case CALL: case RET: case JUMP: case HALT: case SYSCALL:
			case NEG: case ADD: case BAND: case BOR: case BXOR:
			case DIV: case LSH: case MOD: case MUL: case RSH: case SUB:
/*
			case LOADLP: case LOADFP: case LOADAP: case LOADSP: case LOADVR: case LOADEL:
*/
			case DISCARD: case ASGN: case INDIR:
			/* case SAVESTATE: case NEWSTACK: */
				/* no parameter */
				if (count != 1) error(linenr, "invalid number of parameters (need 0)");
				paramtype = PARAM_NONE;
				value.u = 0;
				break;
/*				
			case ASGN: case INDIR:
				/* depends on type */ /*
				paramtype = PARAM_INT;
				if (type == STRUCT) {
					if (count != 2) error(linenr, "invalid number of parameters (need 1)");
					size = PTR_SIZE;
					value.u = parse_numeric(params[1]);
				} else {
					if (count != 1) error(linenr, "invalid number of parameters (need 0)");
					value.u = 0;
				}
				break;
*/				
/*
			case MARK:
				/* special instruction (debug) *//*
				if (count != 1) error(linenr, "invalid number of parameters (need 0)");
				paramtype = PARAM_INT;
				value.u = 0xF0F0F0F0;
				break;
*/				
		}
	} else {
		printf("Instruction '%s' not recognized\n", params[0], opcode);
		error(linenr, "invalid instruction");
	}
	
	/* force JUMP & CALL type to P4 */
	if (opcode == JUMP || opcode == CALL) {
		size = PTR_SIZE;
		type = POINTER;
	}
	
 	/* make sure it's big enough */
	instruction = malloc(OPCSIZE + PTR_SIZE + size + INT_SIZE);

	/* create & write opcode */
	if ((opcode & 0xFF00) == CV) {
		instruction[0] = opcode >> 8;
		instruction[1] = opcode & 0xFF;
	} else {
		if (!build_opcode(opcode, size, type, instruction)) {
			error(linenr, "invalid instruction type/size");
		}
	}
	MS_Write(segment->data, instruction, OPCSIZE);
	for (i = 0; i < OPCSIZE; i++) MS_WriteBit(segment->mask, FALSE);

	/* create & write parameter */
	switch (paramtype) {
		case PARAM_NONE:
			/* nothing todo */
			break;
		case PARAM_INT:
			/* write numeric value, sizeof int */
			build_parameter(value.u, instruction, INT_SIZE);
			MS_Write(segment->data, instruction, INT_SIZE);
			for (i = 0; i < INT_SIZE; i++) MS_WriteBit(segment->mask, FALSE);
			break;
		case PARAM_CONST:
			/* write numeric value, sizeof instruction */
			build_parameter(value.u, instruction, size);
			MS_Write(segment->data, instruction, size);
			for (i = 0; i < size; i++) MS_WriteBit(segment->mask, FALSE);
			break;
		case PARAM_POINTER:
			/* write pointer (label id) */
			build_parameter(value.u, instruction, PTR_SIZE);
			MS_Write(segment->data, instruction, PTR_SIZE);
			for (i = 0; i < PTR_SIZE; i++) MS_WriteBit(segment->mask, TRUE);
			break;
	}
	
	/* return */
	free(instruction);
	return TRUE;
}

/*
 * parse any value, given it's type and size 
 */
Value parse_value(char* value, int type, int size) {
	Value ret;
	
	switch(type) {
		case POINTER:
			ret.u = (unsigned)parse_numeric(value);
			break;
		case INTEGER:
			ret.i = (int)parse_numeric(value);
			break;
		case UNSIGNED:
			ret.u = (unsigned)parse_numeric(value);
			break;
		case FLOAT:
			ret.f = (float)atof(value);
			break;
		case VOID:
			break;
		case STRUCT:
			break;
		default:
			printf("Value type identifier '%c' not recognized", type);
			error(linenr, "unsupported value type");
	}
	return ret;
}

/* 
 * parse a type 
 */
int parse_type(char type) {
	switch(type) {
		case 'P': 		return POINTER;
		case 'I':			return INTEGER;
		case 'U':			return UNSIGNED;
		case 'F':			return FLOAT;
		case 'V':			return VOID;
		case 'B':			return STRUCT;
		default:			
			printf("Value type identifier '%c' not recognized\n", type);
			error(linenr, "unsupported value type");
	}
}

/*
 * write an error to screen and exit the application 
 */
void error(int line, char* message) {
	printf("ERROR AT LINE %d: %s\n", line, message);
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
