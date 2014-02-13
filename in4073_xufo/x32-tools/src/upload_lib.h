#ifndef UPLOAD_LIB_H
#define UPLOAD_LIB_H

/* min(fpga_in_buffer, fpga_out_buffer, pc_in_buffer, pc_out_buffer) */
#define BUFFER_SIZE 8

/* execute a fpga command */
int exec_command(char* command);
/* upload a file to the fpga */
/*		The first parameter contains the filename to upload, the second is an 
			integer	pointing to the location where to place the executable in the 
			loader memory	(default 0). The third parameter is a boolean (1 or 0) 
			whether the uploader may use stdout to show upload progress. The 
			fourth parameter is another boolean which causes "fast" upload mode
			when it's nonzero. "fast" upload mode does not verify any written
			data.
*/
int upload_file(char*, int, int, int);

/* rs232 send/receive functions */
void set_putchar(void (*)(char));
void set_getchar(int (*)());

#endif
