/* May 23, 2006, contains nested structure bugfix */
/* see textparser.h for general information */
#include "textparser.h"

/*
 * parse a numeric value and simple expressions containing additions and
 * 	subtractions. All numerical values should be in decimal.
 */
/*
 * Thanks to Josef Scheuer for finding and fixing a bug in this function
 */
long parse_numeric(char* str) {
	long res, num;
	int idx, sgn;

	for (res = 0, idx = 0; str[idx] != '\0'; ) {
		/* determine sign */
		for (sgn = 1; str[idx] == '+' || str[idx] == '-'; idx++)
			sgn *= (str[idx] == '+')? 1 : -1;
		
		/* evaluate number */
		for (num = 0; isdigit(str[idx]); idx++)
			num = num * 10 + str[idx] - '0';
	
		res += sgn * num;
	}
	return res;	
}

/*
 *	read one line from a file
 */
int read_line(FILE* file, char* line, int max_length) {
	int pos;				/* position in line */
	BOOL endofline;	/* end of line character found */
	char c;					/* character */
	
	/* initialize */
	endofline = FALSE;
	pos = 0;
	/* read while not end of line, and not maximum number of chars seen */
	while (!endofline && (pos < max_length-1)) {
		/* read character */
		switch(c = fgetc(file)) {
			case EOF:
				/* end of file = end of line */
				endofline = TRUE;
				break;
			case '\r':
				/* ignore, stop on '\n' */
				break;
			case '\n':
				/* return */
				endofline = TRUE;
				break;
			case '\0':
				/* should be a text file!! */
				endofline = TRUE;
				break;
			default:
				/* char ok */
				line[pos++] = c;
				break;
		}
	}

	/* terminate the string */
	line[pos] = 0; 
 
 	/* if end of file and no line read, return -1 */
	if (pos == 0 && c == EOF) { 
		return -1; 
	} else {
		return pos;
	}
} 

/*
 * remove comment, and unwanted characters at beginning
 *	or end of a line
 */
void trim_line(char* line) {
	int i, j;
	
	/* find first real character */
	i = j = 0;
	while (line[i] == ' ' || line[i] == '\t') {
		i++;
	}
	/* find last real character and move line left (if needed) */
	while (line[j] != 0 && line[j] != '#') {
		line[j] = line[i+j]; 
		j++;
	}
	j-=i;
	/* remove ending */
	while (line[j-1] == '\t' || line[j-1] == ' ') {
		j--;
	}
	/* terminate */
	line[j] = 0;
}

/*
 *	split a line into a char[], splitted on tabs and spaces
 *		note that no copy is made: after split_line() line
 *		will be the same as items[0] (and will no longer
 *		contain the whole line)
 */
int split_line(char* line, char* items[]) {
	int i, j;
	
	/* set items[0] to first part */
	items[0] = line;
	
	i = j = 0;
	while(line[i] != 0) {
		/* loop throught the line, if a split character is found;
				set it to '\0' and point the next item to the next character
		*/
		switch(line[i]) {
			case ' ': case '\t':
				items[++j] = &line[i+1];
				line[i] = '\0';
				break;
		}
		i++;
	}
	/* return number of items found (at least one) */
	return j+1;
}
