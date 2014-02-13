/*
 * bug: unresolved non imported labels!
 * check for segment == UNKNOWN?
 */

/* see linker.h for general information */
#include "linker.h"
#include "library.h"
#include "x32_version.h"
#include "segments.h"
#include <stdlib.h>
#include <string.h>

/*
 * Application entry point
 */
int main(int argc, char** argv) {
	/* file pointer for input and output files */
	FILE *file, *output_file;
	/* filenames */
	char *filename, *output_filename, *dbg_filename, *bootstrap_file;
	/* loop variables */ 
	int i, j;
	/* binary copy buffer */
	unsigned char *data;
	/* list for holding input files */
	List* files;
	/* list for holding the assemblies */
	/* the first assembly added to the list should be the bootstrap code,
				the last the assembly containing the fictional '$endprog' label */
	List* assemblies;
	/* list enumeration variable */
	ListEnumeration* lenum;
	/* hashtable containing all existing library functions */
	HashTable* library_functions;
	/* hashtable containing all (public) functions currently added to 
				the assemblies list */
	HashTable* public_functions;
	/* hashtable enumeration variable */
	HashTableEnumeration *henum, *henum2;
	/* assembly structure for loading assemblies */
	Assembly* assembly;
	/* assembly structure for holding the bootstrap code */
	Assembly* bootstrap;
	/* fictional '$endprog' label */
	Label* bss_size_pointer;
	/* enumeration label */
	Label *label, *label2;
	/* base address */
	int base_address;
	/* processor version id */
	int proc_version_id;
	/* size of bss segment */
	int bss_size;
	char bss_size_bigendian[4];

	/* command line switches */
	BOOL is_output_file, is_library, import_lib, verbose, 
		is_base_address, is_bootstrap_file,	is_version_id;
	
	/* create hashtables and lists */
	library_functions = HT_Create();
	public_functions = HT_Create();
	files = LST_Create();
	assemblies = LST_Create();
	
	/* initialize variables */
	is_output_file = is_library = import_lib = verbose = is_base_address = 
		is_bootstrap_file = is_version_id = FALSE;
	output_file = 0;
	output_filename = 0;
	base_address = 0;
	bootstrap_file = 0;
	proc_version_id = 0;
	
	/* parse the command line */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			/* switch */
			if (strcmp(&argv[i][1], "o") == 0) {
				is_output_file = TRUE;
			} else if (strcmp(&argv[i][1], "buildlib") == 0) {
				is_library = TRUE;
			} else if (strcmp(&argv[i][1], "lib") == 0) {
				import_lib = TRUE;
			} else if (strcmp(&argv[i][1], "v") == 0) {
				verbose = TRUE;
			} else if (strcmp(&argv[i][1], "base") == 0) {
				is_base_address = TRUE;
			} else if (strcmp(&argv[i][1], "b") == 0) {
				is_bootstrap_file = TRUE;
			} else if (strcmp(&argv[i][1], "vid") == 0) {
				is_version_id = TRUE;
			} else {
				printf("Invalid switch %s\n", argv[i]);
				printUsage();
				exit(EXIT_FAILURE);
			}
		} else if (import_lib) {
			// check if it is a directory, or a file
			j = strlen(argv[i]);
			if (((argv[i][j-1]) == 'l' || (argv[i][j-1]) == 'L') &&
				((argv[i][j-2]) == 'c' || (argv[i][j-2]) == 'C') &&
				((argv[i][j-3]) == '.')) {

				if (load_libfile(argv[i], library_functions)) {
					/* loading ok */
				} else {
					printf("Could not load %s\n", argv[i]);
				}
			} else {
				if (load_libdir(argv[i], library_functions)) {
					/* loading ok */
				} else {
					printf("Could not load %s\n", argv[i]);
				}
			}
			import_lib = FALSE;
		} else if (is_output_file) {
			/* output file name */
			output_filename = argv[i];
			is_output_file = FALSE;
		} else if (is_bootstrap_file) {
			/* location file name */
			bootstrap_file = argv[i];
			is_bootstrap_file = FALSE;
		} else if (is_base_address) {
			/* base address */
			base_address = hex2bin(argv[i]);
			if (base_address < 0) {
				printf("Invalid base address, must be hexadecimal\n");
				base_address = 0;
			}
			is_base_address = FALSE;
		} else if (is_version_id) {
			/* version id */
			if (proc_version_id == 0) {
				if ((proc_version_id = hex2bin(argv[i])) < 0) {
					printf("Invalid version ID (must be hexadecimal)\n");
					printUsage();
					exit(-1);
				}
			} else {
				printf("Multiple version id's given\n");
				printUsage();
				exit(-1);
			}
			is_version_id = FALSE;
		} else {
			/* store filename */
			LST_Add(files, argv[i]);
		}
		 
	}
	if (proc_version_id == 0x0000) proc_version_id = DEF_PROC_VERSION_ID;

	/* check if output file was specified */
	if (output_filename == 0) {
		printf("No outputfile specified\n");
		printUsage();
		exit(EXIT_FAILURE);
	}
	
	if (LST_GetCount(files) == 0) {
		printf("No input files specified\n");
		printUsage();
		exit(EXIT_FAILURE);
	}

	/* see if a library, or an executable must be generated */
	if (is_library) {

		/* open output file */
		output_file = fopen(output_filename, "wb");
		if (!output_file) {
			printf("Could not open output file '%s'\n", argv[i]);
			printUsage();
			exit(EXIT_FAILURE);
		}

		/* write magic marker */
		fputc('S', output_file); fputc('L', output_file); fputc('I', output_file); fputc('B', output_file);
		/* write amount of assemblies in library */
		i = LST_GetCount(files);
		fwrite(&i, sizeof(int), 1, output_file);
		
		/* allocate copy buffer */
		data = (unsigned char*)malloc(BUFFERSIZE);

		/* enumerate over input files */
		lenum = LST_StartEnumeration(files);
		
		while(filename = (char*)LST_GetNextItem(lenum)) {
			/* open file */
			file = fopen(filename, "rb");
			if (!file) {
				printf("Could not open source file '%s'\n", filename);
				printUsage();
				exit(EXIT_FAILURE);
			}
						
			/* copy */
			while (j = fread(data, 1, BUFFERSIZE, file)) {
				fwrite(data, j, 1, output_file);
			}
			fclose(file);
		}
		
		LST_StopEnumeration(lenum);
		/* destroy copy buffer */
		free(data);
		/* close output file */
		fclose(output_file);
	} else {
		/* load the bootstrap assembly, register it's public functions
					in the public functions hashtable	 */
		if (!bootstrap_file) bootstrap_file = "bootstrap.co";
		bootstrap = load_loader(public_functions, bootstrap_file);
		if (!bootstrap) {
			printf("Could not load bootstrap code. Make sure 'bootstrap.co' exists\n");
			exit(EXIT_FAILURE);
		}

		/* add bootstrap first */
		LST_Add(assemblies, bootstrap);

		/* create the fictional label */
		bss_size_pointer = create_label("$endprog");
		bss_size_pointer->segment = BSS;
		/* register the label, but don't add the assembly containing it yet
					(should be added last) */
		HT_Add(public_functions, bss_size_pointer, bss_size_pointer->name);

		/* load all assemblies */
		lenum = LST_StartEnumeration(files);
		
		while(filename = (char*)LST_GetNextItem(lenum)) {
			/* open file */
			file = fopen(filename, "rb");
			if (!file) {
				printf("Could not open source file '%s'\n", argv[i]);
				printUsage();
				exit(EXIT_FAILURE);
			}
			
			/* load assembly, register public functions/variables */
			assembly = load_assembly(file, public_functions, filename);
			assembly->source = (char*)malloc(strlen(filename)+1);
			strcpy(assembly->source, filename);
			/* add assembly to assemblies list */
			LST_Add(assemblies, assembly);
			fclose(file);
		}
		
		LST_StopEnumeration(lenum);

		/* resolve all labels, automatically loads library assemblies when required */
		if (!resolve_labels(assemblies, public_functions, library_functions)) {
			error("Unresolved labels");
		}
		
		/* add the bss size assembly, last in file */
		LST_Add(assemblies, create_single_label_assembly(bss_size_pointer));
		
		/* now the size of the bss segment is known, update $bss_size */
		/* find size: */
		bss_size = 0;
		lenum = LST_StartEnumeration(assemblies);
		while(assembly = LST_GetNextItem(lenum)) bss_size += assembly->bss_size;
		LST_StopEnumeration(lenum);

		/* resolve all addresses */
		recomp_addresses(assemblies, base_address);

		/* update all code segments */
		lenum = LST_StartEnumeration(assemblies);
		while(assembly = LST_GetNextItem(lenum)) {
			/* all segments can contain labels! */
			update_segment(assembly, assembly->code);
			update_segment(assembly, assembly->data);
			update_segment(assembly, assembly->lit);
		}
		LST_StopEnumeration(lenum);
		
		/* print public functions and variable locations to stdout */
		if (verbose) printPublics(public_functions, base_address);
		
		/* open output file */
		output_file = fopen(output_filename, "wb");
		if (!output_file) {
			printf("Could not open output file '%s'\n", argv[i]);
			printUsage();
			exit(EXIT_FAILURE);
		}
		
		/* write code segment */
		lenum = LST_StartEnumeration(assemblies);
		
		while(assembly = LST_GetNextItem(lenum)) {
			fseek(output_file, assembly->code->offset-base_address, SEEK_SET);
			MS_Dump(assembly->code->data, output_file);
		}
		LST_StopEnumeration(lenum);

		/* write lit segment */
		lenum = LST_StartEnumeration(assemblies);

		while(assembly = LST_GetNextItem(lenum)) {
			fseek(output_file, assembly->lit->offset-base_address, SEEK_SET);
			MS_Dump(assembly->lit->data, output_file);
		}
		LST_StopEnumeration(lenum);

		/* write data segment */
		lenum = LST_StartEnumeration(assemblies);
		
		while(assembly = LST_GetNextItem(lenum)) {
			fseek(output_file, assembly->data->offset-base_address, SEEK_SET);
			MS_Dump(assembly->data->data, output_file);
		}
		LST_StopEnumeration(lenum);

		/* close file */
		fclose(output_file);
		output_file = NULL;

		/* check if there are debugging symbols present */
		henum = HT_StartEnumeration(public_functions);
		while(label = (Label*)HT_GetNextItem(henum)) {
			if (output_file == NULL && label->debuginfo != LABEL_DEBUG_NONE) {
				dbg_filename = (char*)malloc(strlen(output_filename) + strlen(DEBUG_EXT) + 2);
				replace_ext(output_filename, dbg_filename, DEBUG_EXT);
				output_file = fopen(dbg_filename, "wb");
				if (output_file == NULL) {
					printf("Could not open '%s'. Debug symbols not written.\n", dbg_filename);
				}
			}
		}
		HT_StopEnumeration(henum);

		if (output_file) {
			henum = HT_StartEnumeration(public_functions);
			while(label = (Label*)HT_GetNextItem(henum)) {
				switch(label->debuginfo) {
					case LABEL_DEBUG_NONE:
						break;
					case LABEL_DEBUG_LINE:
						fprintf(output_file, "line: %s @ 0x%08X\n", label->name+1, label->location);
						break;
					case LABEL_DEBUG_FILE:
						fprintf(output_file, "file: %s @ 0x%08X\n", label->name+1, label->location);
						break;
					case LABEL_DEBUG_FUNCTION:
						fprintf(output_file, "function: %s @ 0x%08X\n", label->name+1, label->location);
						break;
					case LABEL_DEBUG_GLOBAL:
						fprintf(output_file, "global: %s @ 0x%08X\n", label->name+1, label->location);
						break;
					case LABEL_DEBUG_LOCAL:
						fprintf(output_file, "local: %s @ 0x%08X\n", label->name+1, label->location);
						break;
					case LABEL_DEBUG_PARAM:
						fprintf(output_file, "parameter: %s @ 0x%08X\n", label->name+1, label->location);
						break;
					case LABEL_DEBUG_TYPEDEF:
						fprintf(output_file, "typedef: %s\n", label->name+1);
						break;
					case LABEL_DEBUG_FIELD:
						fprintf(output_file, "field: %s @ 0x%08X\n", label->name+1, label->location);
						break;
				}
			}
			HT_StopEnumeration(henum);


			/* create debug symbols for all labels, if they don't exist yet, this 
				creates the addresses for all functions & global variables in files
				which are not compiled with -g (eg, the library) */
			data = (unsigned char*)malloc(1024);
			henum = HT_StartEnumeration(public_functions);
			while(label = (Label*)HT_GetNextItem(henum)) {
				if (label->debuginfo == LABEL_DEBUG_NONE && label->exported && strlen(label->name) < 1023) {
					sprintf((char*)data, "$%s", label->name);
	
					i = 0;
					henum2 = HT_StartEnumeration(public_functions);
					while(label2 = (Label*)HT_GetNextItem(henum2)) {
						if (!strncmp((char*)data, label2->name, strlen((char*)data))) {
							i = 1;
							break;
						}
					}
					HT_StopEnumeration(henum2);
					if (!i) {
						if (label->segment == CODE) {
							fprintf(output_file, "function: %s <0> @ 0x%08X\n", label->name, label->location);
						} else {
							fprintf(output_file, "global: %s @ <0> 0x%08X\n", label->name, label->location);
						}
					}
				}
			}
			HT_StopEnumeration(henum);
			free(data);
			free(dbg_filename);
		} 
	}
	
	HT_Destroy(library_functions);
	
	return 0;
}

/* 
 * print linker usage to stdout
 */
void printUsage() {
	printf("Usage:\n");
	printf("\tlinker inputfile [inputfile...] -o outputfile [-buildlib] [-b bootstrap_file] [-base base_address] [[-lib librarydir]...]\n\n");
	printf("\tinputfile:      input file location\n");
	printf("\tlibrarydir:			directory containing library files\n");
	printf("\toutputfile:     output file location\n");
	printf("\tbootstrapfile:  location of bootstrap file\n");
	printf("\tbuildlib:       build library instead of executable\n");
	printf("\tbase_address:   increase all addresses with base_address (hexadecimal)\n");
}

/*
 * load a binary assembly file into an assembly structure 
 *	registers all public labels in the hashtable
 */
Assembly* load_assembly(FILE* file, HashTable* public_functions, char* filename) {
	Assembly* assembly;		/* assembly structure */
	int i, count, size;	
	char c;
	char key[4];
	Label* label;
	 
	/* allocate space for the structure */
	assembly = (Assembly*)malloc(sizeof(Assembly));
	/* create hashtable for labels */
	assembly->labels = HT_Create();
	/* reset label count */
	assembly->label_count = 0;
	/* reset source */
	assembly->source = 0;
	
	/* read magic marker */
	fread(key, 4, 1, file);
	
	/* should be 'SOBJ' */
	if (key[0] != 'S' || key[1] != 'O' || key[2] != 'B' || key[3] != 'J') {
		printf("Corrupt file: %s:0x%04X\n", filename, ftell(file)-4);
		error("Invalid key");
	}
	
	/* read the labels */
	/* see assembler.h for format */
	fread(&count, sizeof(int), 1, file);
	for (i = 0; i < count; i++) {
		label = (Label*)malloc(sizeof(Label));
		size = fgetc(file);
		label->name = (char*)malloc(size+1);
		fread(label->name, size, 1, file);
		label->name[size] = 0;

		switch((c = fgetc(file)) & 0x0F) {
			case 0:
				label->exported = FALSE;
				label->imported = FALSE;
				break;
			case 1:
				/* register the label as public */
				HT_Add(public_functions, label, label->name);
				label->exported = TRUE;
				label->imported = FALSE;
				break;
			case 2:
				label->exported = FALSE;
				label->imported = TRUE;
				break;
			default:
				error("Corrupt file (Invalid label)");
		}
		label->debuginfo = (unsigned char)c >> 4;
		label->segment = fgetc(file);
		label->assembly = assembly;
		label->dep = 0;
		label->offset = 0;
		fread(&label->id, sizeof(int), 1, file);
		fread(&label->location, sizeof(int), 1, file);
		
		/* register the label in the assembly */
		HT_Add(assembly->labels, label, label->name);
		assembly->label_count++;
	}
	
	/* read code segment */
	assembly->code = (Segment*)malloc(sizeof(Segment));
	assembly->code->offset = 0;
	assembly->code->segment = CODE;
	fread(&size, sizeof(int), 1, file);
	assembly->code->data = MS_Load(file, size);
	assembly->code->mask = MS_Load(file, size>>3);

	/* read lit segment */
	assembly->lit = (Segment*)malloc(sizeof(Segment));
	assembly->lit->offset = 0;
	assembly->lit->segment = LIT;
	fread(&size, sizeof(int), 1, file);
	assembly->lit->data = MS_Load(file, size);
	assembly->lit->mask = MS_Load(file, size>>3);

	/* read data segment */
	assembly->data = (Segment*)malloc(sizeof(Segment));
	assembly->data->offset = 0;
	assembly->data->segment = DATA;
	fread(&size, sizeof(int), 1, file);
	assembly->data->data = MS_Load(file, size);
	assembly->data->mask = MS_Load(file, size>>3);

	/* read bss segment size */
	fread(&assembly->bss_size, sizeof(int), 1, file);
	
	/* return */
	return assembly;
}

/*
 * print an arror message to stderr and exit
 */
void error(char* message) {
	fprintf(stderr, "ERROR: %s\n", message);
	exit(EXIT_FAILURE);
}

/*
 * print all public labels to stdout
 */
void printPublics(HashTable* labels, int offset) {
	/* enumeration variable */
	HashTableEnumeration* henum;
	/* label pointer */
	Label* label;

	/* print the functions (all labels in CODE segment) */
	printf("\nPublic functions:\n");
	printf("Name:                                      Location (excl offset):\n");
	henum = HT_StartEnumeration(labels);
	while(label = (Label*)HT_GetNextItem(henum)) {
		if (!label->imported && label->exported && label->segment == CODE && label->offset == 0 && label->debuginfo == 0) {
			printf("  %-40s 0x%08X\n", label->name, label->location - offset);
		}
	}
	HT_StopEnumeration(henum);

	/* print the variables (all labels in other segments) */
	printf("\nVariables:\n");
	printf("Name:                                      Location (excl offset):\n");
	henum = HT_StartEnumeration(labels);
	while(label = (Label*)HT_GetNextItem(henum)) {
		if (!label->imported && label->exported && label->segment != CODE && label->offset == 0 && label->debuginfo == 0) {
			printf("  %-40s 0x%08X\n", label->name, label->location - offset);
		}
	}
	HT_StopEnumeration(henum);

	/* print the debug symbols */
	printf("\nDebug symbols:\n");
	printf("Name:                                      Location (excl offset):\n");
	henum = HT_StartEnumeration(labels);
	while(label = (Label*)HT_GetNextItem(henum)) {
		switch(label->debuginfo) {
			case LABEL_DEBUG_NONE:
				break;
			case LABEL_DEBUG_LINE:
				printf("  LINE: %-34s 0x%08X\n", label->name+1, label->location - offset);
				break;
			case LABEL_DEBUG_FILE:
				printf("  FILE: %-34s 0x%08X\n", label->name+1, label->location - offset);
				break;
			case LABEL_DEBUG_FUNCTION:
				printf("  FUNCTION: %-30s 0x%08X\n", label->name+1, label->location - offset);
				break;
			case LABEL_DEBUG_GLOBAL:
				printf("  GLOBAL: %-32s 0x%08X\n", label->name+1, label->location - offset);
				break;
			case LABEL_DEBUG_LOCAL:
				printf("  LOCAL: %-33s 0x%08X\n", label->name+1, label->location);
				break;
			case LABEL_DEBUG_PARAM:
				printf("  PARAMETER: %-29s 0x%08X\n", label->name+1, label->location);
				break;
			case LABEL_DEBUG_FIELD:
				printf("  FIELD: %-33s 0x%08X\n", label->name+1, label->location);
				break;
			case LABEL_DEBUG_TYPEDEF:
				printf("  TYPEDEF: %-31s\n", label->name+1, label->location);
				break;
		}
	}
	HT_StopEnumeration(henum);
} 

/*
 * print all labels with all information (debug)
 */
void printLabels(HashTable* labels) {

	HashTableEnumeration* henum;
	Label* label;

	henum = HT_StartEnumeration(labels);

	printf("\nLabels:\n");
	printf("%-20sID  %-10s  Location:   External:\n", "Name:", "Segment:");
	while(label = (Label*)HT_GetNextItem(henum)) {
		printf("%-15s+%-4d%-4d%-10s  0x%08X     %-3s\n", label->name, label->offset, label->id, segment_name(label->segment), label->location, label->imported ? "yes" : "no");
	}
	HT_StopEnumeration(henum);
}  

/*
 * check all labels, connect imported functions to corresponding exported functions
 *		and include library assemblies when needed 
 */
BOOL resolve_labels(List* assemblies, HashTable* public_functions, HashTable* library_functions) {
	/* enumeration variables */
	ListEnumeration *lenum;
	HashTableEnumeration *henum, *henum2;
	Assembly *assembly;
	Label *label;
	/* name check variable */
	char *s;
	BOOL errors;
	
	errors = FALSE;
	
	/* go over all assemblies */
	lenum = LST_StartEnumeration(assemblies);
	while(assembly = (Assembly*)LST_GetNextItem(lenum)) {
		/* go over all labels in the assembly */
		henum = HT_StartEnumeration(assembly->labels);
		while(label = (Label*)HT_GetNextItem(henum)) {
			if (label->debuginfo == LABEL_DEBUG_NONE) {
				/* check if the label is relative */
				if (s = strchr(label->name, '+')) {
					/* change the label name from labelname+offset to labelname,
							note that the hashtable key will still be labelname+offset,
							so the label will only be found when looking for labelname+offset.
							Looking for labelname thus never finds a relative label (which is
							a good thing!)
					*/
					*s = 0;
					s++;
					label->offset = parse_numeric(s);
					/* first check the current assembly, if not found it must be imported */
					if ((label->dep = HT_Find(assembly->labels, label->name)) == 0) {
						label->imported = TRUE;
					}
				} else if (s = strchr(label->name, '-')) {
					/* same as above */
					*s = 0;
					s++;
					label->offset = -parse_numeric(s);
					/* first check the current assembly, if not found it must be imported */
					if ((label->dep = HT_Find(assembly->labels, label->name)) == 0) {
						label->imported = TRUE;
					}
				}
			}
			if (label->imported) {
				/* needs to be resolved */
				/* look in public functions */
				if (label->dep = (Label*)HT_Find(public_functions, label->name)) {
					/*  defined in other assemblies allready included
					 *	(set label->dep to exporting label)
					 */
				} else {
					/* look in library */
					if ((label->dep = (Label*)HT_Find(library_functions, label->name))) {
						/* link the library function's assembly */
						LST_Add(assemblies, label->dep->assembly);
						/* copy all library functions to the public functions table */
						henum2 = HT_StartEnumeration(label->dep->assembly->labels);
						while(label = (Label*)HT_GetNextItem(henum2)) {
							/* Add may fail when a local function has the same name as a library function,
									the existing (local) function will stay and the library function will be 
									discarded. Before scanning the public_functions however, the function is
									first searched in the current assembly, so library functions calling a 
									library function which has the same name as a local function will still
									call the library function, while local functions will call the local
									version */
							if (label->exported) HT_Add(public_functions, label, label->name);
						}
						HT_StopEnumeration(henum2);
						
					} else {
						/* label not found */
						printf("Unresolved function or variable %s\n", label->name);
						errors = TRUE;
					}
				}
			}
		}
		HT_StopEnumeration(henum);
	}

	LST_StopEnumeration(lenum);
	return !errors;
}

/*
 * compute all label addresses and segment offsets
 */
void recomp_addresses(List* assemblies, int code_offset) {
	/* total segment sizes */
	int total_code, total_bss, total_data, total_lit;
	/* segment offset sizes */
	int start_code, start_bss, start_data, start_lit;
	int i;
	Label* label;
	Label* temp;
	char* s;
	HashTableEnumeration* henum;
	ListEnumeration* lenum;
	Assembly* assembly;
	
	/* initialize variables */
	total_code = total_bss = total_data = total_lit = 0;
	start_code = start_bss = start_data = start_lit = 0;
	
	/* calculate the total size of all four segments */
	lenum = LST_StartEnumeration(assemblies);
	while(assembly = LST_GetNextItem(lenum)) {
		total_code += assembly->code->data->size;
		total_data += assembly->data->data->size;
		total_lit += assembly->lit->data->size;
		total_bss += assembly->bss_size;
	}
	LST_StopEnumeration(lenum);
	
	/* order: code (read only, initialized)
						lit (read only, initialized)
						data (initialized)
						bss (uninitialized) 
	*/
	/*
	start_code = code_offset;
	start_lit = start_code + total_code;
	start_data = start_lit + total_lit;
	start_bss = start_data + total_data;
	*/
	/*! NEW: using segments.c */
	get_segment_offsets(code_offset, total_code, total_lit, total_data, 
		total_bss, &start_code, &start_lit, &start_data, &start_bss);

	/* set all segment offsets */
	lenum = LST_StartEnumeration(assemblies);
	while(assembly = LST_GetNextItem(lenum)) {
		assembly->code->offset = start_code;
		start_code += assembly->code->data->size;
		assembly->lit->offset = start_lit;
		start_lit += assembly->lit->data->size;
		assembly->data->offset = start_data;
		start_data += assembly->data->data->size;
		assembly->bss_offset = start_bss;
		start_bss += assembly->bss_size;
	}
	LST_StopEnumeration(lenum);

	/* update all label locations */
	lenum = LST_StartEnumeration(assemblies);
	while(assembly = LST_GetNextItem(lenum)) {
		henum = HT_StartEnumeration(assembly->labels);
		while(label = HT_GetNextItem(henum)) {
			/* update */
			update_label_location(label);
		}
		
		HT_StopEnumeration(henum);
	}
	LST_StopEnumeration(lenum);

}

/*
 * update the location of a label 
 */
void update_label_location(Label* label) {
	Label* lbl;
	int location;

	/* find the 'master' label */
	lbl = label;
	location = 0;
	while (lbl->dep) {
		location += lbl->offset;
		lbl = lbl->dep;
	}

	/* set the segment 	and the location */
	label->segment = lbl->segment;
	location += lbl->location;
	
	if (lbl->assembly != 0) {
		/* otherwise the location is absolute  */
		switch(lbl->segment) {
			case CODE:
				location += lbl->assembly->code->offset;
				break;
			case LIT:
				location += lbl->assembly->lit->offset;
				break;
			case DATA:
				location += lbl->assembly->data->offset;
				break;
			case BSS:
				location += lbl->assembly->bss_offset;
				break;
			case BASE:
				break;
		}
	}

	/* store the label */
	label->location = location;
	/* the label address is now absolute, destroy any references */
	label->assembly = 0;
	label->dep = 0;
	label->offset = 0;
}

/*
 * find a label in a table (critical)
 */
Label* find_label(HashTable* labels, char* name) {
	Label* label;
	
	/* find */
	label = HT_Find(labels, name);
	
	if (label) return label;
	
	/* not found, crash */
	printf("Unresolved function %s\n", name);
	error("Label not found");
	return 0;
}
 
/*
 * find a label by its id 
 */
Label* find_label_by_id(Assembly* assembly, int id) {
	HashTableEnumeration* henum;
	Label* label;
	
	/* enumerate the labels */
	henum = HT_StartEnumeration(assembly->labels);
	while(label = HT_GetNextItem(henum)) {
		if (label->id == id) {
			/* found */
			HT_StopEnumeration(henum);
			return label;
		}
	}
	HT_StopEnumeration(henum);

	/* not found, crash */
	printf("Unresolved label #%d\n", id);
	error("Label not found (corrupt object file)");
	return 0;
}

/*
 * update a binary segment with label addresses
 */
void update_segment(Assembly* assembly, Segment* segment) {
	int pos;						/* current position in segment */
	int id;							/* address' label id */
	int i;							/* looping variable */
	int location;				/* label location */
	unsigned char c;		/* binary bitmap data */
	
	int counter;				/* one-counter */
	
	/* initialize */
	pos = counter = 0;
	/* read the bitmap segment */
	while (MS_Read(segment->mask, &c, 1)) {
		
		/* one byte = 8 bits = 8 bytes in binary data */
		if (c != 0) {
			/* read bit by bit */
			for (i = 0; i < 8; i++) {
				/* check if it's a one */
				if (!counter && (c & 0x80)) {
					/* next PTR_SIZE should be ones as well */
					
					/* seek the segment to current position */
					MS_Seek(segment->data, pos+i);
					/* read PTR_SIZE bytes, the segment should hold the label id */
					id = MS_ReadBE(segment->data, PTR_SIZE);

					/*
					printf("Label reference detected at %s:0x%04X (ID: 0x%02X)\n", assembly->source, pos+i, id);
					*/

					/* seek back */
					MS_Seek(segment->data, pos+i);
					/* write the label location */
					MS_WriteBE(segment->data, find_label_by_id(assembly, id)->location, PTR_SIZE);
					
					counter = PTR_SIZE;
				}
				
				/* next bit */
				c=c<<1;
				if (counter) counter--;
			}
		} else {
			counter -= 8;
			if (counter < 0) counter = 0;
		}

		/* update position */
		pos += 8;
	}
}

/*
 * create a label with only a name
 */
Label* create_label(char* name) {
	Label* label;
	label = (Label*)malloc(sizeof(Label));
	label->name = (char*)malloc(strlen(name)+1);
	strcpy(label->name, name);
	label->segment = DATA;
	label->location = 0;
	label->assembly = 0;
	label->id = 1;
	label->dep = 0;
	label->offset = 0;
	label->debuginfo = LABEL_DEBUG_NONE;
	label->exported = TRUE;
	label->imported = FALSE;
	return label;
}

/*
 * create an empty assembly containing only one label
 */
Assembly* create_single_label_assembly(Label* label) {
	Assembly* assembly;
	assembly = (Assembly*)malloc(sizeof(Assembly));
	assembly->code = (Segment*)malloc(sizeof(Segment));
	assembly->code->offset = 0;
	assembly->code->segment = CODE;
	assembly->code->data = MS_Open(0);
	assembly->code->mask = MS_Open(0);
	assembly->lit = (Segment*)malloc(sizeof(Segment));
	assembly->lit->offset = 0;
	assembly->lit->segment = LIT;
	assembly->lit->data = MS_Open(0);
	assembly->lit->mask = MS_Open(0);
	assembly->data = (Segment*)malloc(sizeof(Segment));
	assembly->data->offset = 0;
	assembly->data->segment = DATA;
	assembly->data->data = MS_Open(0);
	assembly->data->mask = MS_Open(0);
	assembly->bss_size = 0;
	assembly->bss_offset = 0;
	assembly->labels = HT_Create();
	assembly->label_count = 1;
	assembly->source = 0;
	label->assembly = assembly;
	HT_Add(assembly->labels, label, label->name);
	return assembly;
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

/* 
 * replace extension from original file with newext
 *  (make sure dbgfile buffer is at least strlen(exefile)+strlen(newext)+1 to
 *  add .ext when no extension is found
 */
void replace_ext(char* exefile, char* dbgfile, char* newext) {
	char *fromslash, *frombslash, *fromdot;
	strcpy(dbgfile, exefile);

	fromslash = strrchr(dbgfile, '/');
	frombslash = strrchr(dbgfile, '\\');
	fromdot = strrchr(dbgfile, '.');

	if (fromdot > fromslash && fromdot > frombslash) {
		strcpy(fromdot, newext);
	} else {
		strcat(dbgfile, newext);
	}
}
