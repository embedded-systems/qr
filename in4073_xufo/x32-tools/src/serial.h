/* open serial port, if 2nd parameter is 0, it is opened in non-blocking 
	mode, return port handle */
void* rs232_open(char*, int);
/* read from serial port, first parameter is the port handle, second a 
	character buffer, and third the amount of bytes to read. returns the
	amount of bytes read */
int rs232_read(void*, void*, int);
/* write to serial port, first parameter is the port handle, second the
	character buffer, and third the amount of bytes to write. returns the
	amount of bytes written */
int rs232_write(void*, void*, int);
/* close a serial port (takes handle) */
void rs232_close(void*);
