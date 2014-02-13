
#ifndef SHELL_H
#define SHELL_H

#define MAX_LINE_LENGTH			256

#define MAX_ARGS					8
#define MIN(a, b)					(a<b?a:b)
#define MAX(a, b)					(a>b?a:b)


typedef struct {
	char line[MAX_LINE_LENGTH];
	char arg_buffer[MAX_LINE_LENGTH];
	char* args[MAX_ARGS];
	int line_length;
	int arg_count;
	int cursor_position;
} TERMINAL;

void open_terminal(TERMINAL*);
void get_command(TERMINAL*);
void close_terminal(TERMINAL*);


#define PRINTF_BUFFER_SIZE 256
#define printf stripped_printf

int stripped_printf(const char* fmt, ...);

#endif
