/*
 * Textparser: Contains functions to read and parse a text file
 *
 *	Author: Sijmen Woutersen
 */

#ifndef TEXTPARSER_H
#define TEXTPARSER_H

#include <stdio.h>
#include "bool.h"

/* read one line from a file */
int read_line(FILE*, char*, int);
/* remove leading spaces, tabs and any comment (#)*/
void trim_line(char*);
/* split the line on spaces and tabs */
/* note: original line is lost */
int split_line(char*, char**);
/* parse a numeric value */
long parse_numeric(char*);


#endif
