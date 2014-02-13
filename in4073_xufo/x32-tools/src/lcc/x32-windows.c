/* x32 */

#include <string.h>

static char rcsid[] = "$Id: x32.c,v 0.2.0 2006/04/25 15:24:47 drh Exp $";

#define DEFAULT_VERSION_ID "0021"

#ifndef LCCDIR
#define LCCDIR ""
#endif

char *suffixes[] = { ".c;.C", ".i;.I", ".bc;.BC", ".co;.CO", ".ce;.cl;.CE;.CL", 0 };
char inputs[256] = "";
char *cpp[] = { LCCDIR "cpp", "-D_X32", "-D_VID=0x" DEFAULT_VERSION_ID, "$1", "$2", "$3", 0 };
char *include[] = { "-I" LCCDIR "..\\lib-x32", 0 };
char *com[] = { LCCDIR "rcc", "", "-vid=" DEFAULT_VERSION_ID, "-target=xbytecode", "$1", "$2", "$3", 0 };
char *as[] = { LCCDIR "x32-asm", "-vid", DEFAULT_VERSION_ID, "-o", "$3", "$1", "$2", 0 };
char *ld[] = { LCCDIR "x32-link", "-vid", DEFAULT_VERSION_ID, "-base", "0", "", "-lib", LCCDIR "..\\lib-x32", "-b", LCCDIR "\\bootstrap.co", "$2", "-o", "$3", "$1", 0};

extern char *concat(char *, char *);
extern char *replace(const char *, int, int);

/* variable to check whether a custom bootstrap file is used, if so,
			don't update the bootstrap file location with the -lccdir
			parameter */
int custom_bootstrap = 0;
int custom_libdir = 0;

int option(char *arg) {

	if (strncmp(arg, "-lccdir=", 8) == 0) {

		if (arg[strlen(arg)-1] == '\\' || arg[strlen(arg)-1] == '/')
			arg[strlen(arg)-1] = '\0';
			
		cpp[0] = concat(&arg[8], "\\cpp");
		include[0] = concat("-I", concat(&arg[8], "/../lib-x32"));
		com[0] = concat(&arg[8], "\\rcc");
		as[0] = concat(&arg[8], "\\x32-asm");
		ld[0] = concat(&arg[8], "\\x32-link");
		if (!custom_bootstrap)
			ld[9] = concat(&arg[8], "\\bootstrap.co");		
		if (!custom_libdir) 
			ld[7] = concat(&arg[8], "\\..\\lib-x32");				

	} else if (strcmp(arg, "-buildlib") == 0) {
		ld[5] = arg;
	} else if (strncmp(arg, "-g", 2) == 0) {
		com[1] = arg;
	} else if (strncmp(arg, "-vid=", 5) == 0) {
		com[2] = concat("-vid=", &arg[5]);
		as[2] = &arg[5];
		ld[2] = &arg[5];
		cpp[2] = concat("-D_VID=0x", &arg[5]);
	} else if (strncmp(arg, "-bootstrap=", 11) == 0) {
		ld[9] = &arg[11];
		/* make sure the -bootstrap parameter isn't overruled by -lccdir parameter */
		custom_bootstrap = 1;
	} else if (strncmp(arg, "-base=", 6) == 0) {
		ld[4] = &arg[6];
	} else if (strncmp(arg, "-libdir=", 8) == 0) {
		ld[7] = &arg[8];
		custom_libdir = 1;
	}	else {
		return 0;
	}
	return 1;
}
