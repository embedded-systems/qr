#include <stdio.h>
#include "serial.h"
#ifdef _WIN32
#include <windows.h>

#define BAUDRATE 		115200
#define DATABITS		8
#define STOPBITS 		ONESTOPBIT
#define PARITY			NOPARITY
#define RTSCONTROL	RTS_CONTROL_DISABLE

int debug = 0;

HANDLE initCOM(char*, int);

void* rs232_open(char* comport, int blocking) {
	return (void*)initCOM(comport, blocking);
}

void rs232_close(void* com) {
	CloseHandle((HANDLE)com);
}

int rs232_read(void* com, void* buffer, int bytes) {
	DWORD count;
	ReadFile((HANDLE)com, buffer, bytes, &count, (LPOVERLAPPED)0);
	return (int)count;
}

int rs232_write(void* com, void* buffer, int bytes) {
	DWORD count;
	WriteFile((HANDLE)com, buffer, bytes, &count, (LPOVERLAPPED)0);
	return (int)count;
}


HANDLE initCOM(char* comport, int blocking) {
	HANDLE hcom;
	COMMTIMEOUTS cto;

	//open comport, use mostly default properties
	hcom = CreateFile(comport, GENERIC_READ | GENERIC_WRITE, 
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hcom == INVALID_HANDLE_VALUE) return 0;

	if (GetCommTimeouts(hcom, &cto)) {
		if (blocking) {
			cto.ReadIntervalTimeout = 0;
			cto.ReadTotalTimeoutConstant = 0;
			cto.ReadTotalTimeoutMultiplier = 0;
		} else {
			cto.ReadIntervalTimeout = MAXDWORD;
			cto.ReadTotalTimeoutConstant = 0;
			cto.ReadTotalTimeoutMultiplier = 0;
		}
		
		if (!SetCommTimeouts(hcom, &cto)) {
			CloseHandle(hcom);
			return 0;
		}
	} else {
		CloseHandle(hcom);
		return 0;
	}
	

	
	//set comm settings (baudrate eg) (see updateCommState())
	if (!updateCommState(hcom)) {
		CloseHandle(hcom);
		return 0;
	}
	
	return hcom;
}

int updateCommState(HANDLE hcom) {
	DCB dcb;		//comport settings
	
	//check comport handle
	if (hcom == 0) {
		return FALSE;
	} else {
		dcb.DCBlength = sizeof(dcb);
		//get current comport settings
		if (!GetCommState(hcom, &dcb)) {
			//not a comport, probably a file provided
			CloseHandle(hcom);
			return FALSE;
		} else {
			dcb.BaudRate = BAUDRATE;						//bautrate
			dcb.ByteSize = DATABITS;						//nr of bits
			dcb.StopBits = STOPBITS;						//stop bits
			dcb.Parity = PARITY;								//parity
			dcb.fRtsControl = RTSCONTROL;				//RTS control	
			
			if (!SetCommState(hcom, &dcb) || !SetupComm(hcom, 1024, 1024)) {
				//failure
				return 0;
			} else {
				return 1;
			}
		}
	}
}
#else // _WIN32
/*------------------------------------------------------------
 * serial I/O
 *------------------------------------------------------------
 */
#include <termios.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

void* rs232_open(char* port, int blocking) {
	char *name;
	int result;  
	int fd_RS232;
	struct termios tty;
  
  fd_RS232 = open(port, O_RDWR | O_NOCTTY);
  if (!fd_RS232) return 0;
	
	if (!isatty(fd_RS232) | !ttyname(fd_RS232) | (result = tcgetattr(fd_RS232, &tty))) {
		close(fd_RS232);
		return 0;
	}

	tty.c_iflag = IGNBRK; /* ignore break condition */
	tty.c_oflag = 0;
	tty.c_lflag = 0;
	
	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; /* 8 bits-per-character */
	tty.c_cflag |= CLOCAL | CREAD; /* Ignore model status + read input */
	
	cfsetospeed(&tty, B115200); /* set output baud rate */
	cfsetispeed(&tty, B115200); /* set input baud rate */
	
	tty.c_cc[VMIN]  = 0;
	tty.c_cc[VTIME] = 0;
	
	tty.c_iflag &= ~(IXON|IXOFF|IXANY);
	
	result = tcsetattr (fd_RS232, TCSANOW, &tty); /* non-canonical */
	
	tcflush(fd_RS232, TCIOFLUSH); /* flush I/O buffer */
          	
	return (void*)fd_RS232;
}


void rs232_close(void* comhandle) {
	close((int)comhandle);
}


int rs232_read(void* comhandle, void* buffer, int size) {
	return read((int)comhandle, buffer, size);
}

int rs232_write(void* comhandle, void* buffer, int size) {
	return write((int)comhandle, buffer, size);
}

#endif // _WIN32
