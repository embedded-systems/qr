/* see library.h for general information */
#include "library.h"
#include "linker.h"
#include <stdio.h>
#include <math.h>

#include <dirent.h>
#include <string.h>

char *strlwr(char*);

/*
 * load the loader file
 *		all public labels are stored in the labels parameter
 */
Assembly* load_loader(HashTable* labels, char* filename) {
	Assembly* assembly;
	FILE* file;
	
	/* open file */
	file = fopen(filename, "rb");
	if (!file) return 0;

	/* load */
	assembly = load_assembly(file, labels, filename);
	assembly->source = "<loader>";
	
	fclose(file);
	return assembly;
}

/*
 *	load a library directory,
 *		all public labels are stored in the labels parameter
 */
BOOL load_libdir(char* directoryname, HashTable *labels) {
	char* name;
	struct dirent *dp;
	DIR *dfd;
	
	if (!(dfd = opendir(directoryname))) {
		return FALSE;
	}
	while (dp = readdir(dfd)) {
		if (strcmp(dp->d_name, directoryname) && strcmp(dp->d_name, "..")) {
			if (strcmp((char*)strlwr((char*)(dp->d_name+strlen(dp->d_name)-3)), ".cl") == 0) {
				
				name = (char*)malloc(strlen(directoryname) + strlen(dp->d_name) + 2);
			
				sprintf(name, "%s/%s", directoryname, dp->d_name);
				load_libfile(name, labels);
				free(name);
	
			}


		}
	}
	closedir(dfd);
	return TRUE;
}

/*
 *	load a library file,
 *		all public labels are stored in the labels parameter
 */
BOOL load_libfile(char* filename, HashTable* labels) {
	FILE* file; 				/* library file handle */
	Assembly* assembly;	/* assembly ptr */
	int i, count;				/* loop variables */
	
	/* open file */
	file = fopen(filename, "rb");
	if (!file) return FALSE;
	
	/* check key */
	if (fgetc(file) != 'S' || fgetc(file) != 'L' || fgetc(file) != 'I' || fgetc(file) != 'B') {
		fclose(file);
		return FALSE;
	}
	
	/* read nr of onjects */
	fread(&count, sizeof(int), 1, file);
	
	/* load the objects */
	for (i = 0; i < count; i++) {
		assembly = load_assembly(file, labels, filename);
		assembly->source = (char*)malloc(strlen(filename)+1);
		strcpy(assembly->source, filename);
	}

	/* close file */
	fclose(file);
	
	return TRUE;
}


/* convert a string to lower case */
char *strlwr(char *str) {
	char *s = str;
	while(*s) {
		if (*s >= 'A' && *s <= 'Z') *s = *s - 'A' + 'a';
		s++;
	}
	return str;
}
