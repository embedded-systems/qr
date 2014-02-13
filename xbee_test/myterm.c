/*------------------------------------------------------------
 * Simple terminal in C
 * 
 * Arjan J.C. van Gemund (+ few mods by Mark Dufour)
 *------------------------------------------------------------
 */
#define	FALSE		0
#define	TRUE		1

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

/*------------------------------------------------------------
 * console I/O
 *------------------------------------------------------------
 */
struct termios 	savetty;

void	term_initio()
{
	struct termios tty;

	tcgetattr(0, &savetty);
	tcgetattr(0, &tty);

	tty.c_lflag &= ~(ECHO|ECHONL|ICANON|IEXTEN);
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 0;

	tcsetattr(0, TCSADRAIN, &tty);
}

void	term_exitio()
{
	tcsetattr(0, TCSADRAIN, &savetty);
}

void	term_puts(char *s) 
{ 
	fprintf(stderr,"%s",s); 
}

void	term_putchar(char c) 
{ 
	putc(c,stderr); 
}

int	term_getchar_nb() 
{ 
        static unsigned char 	line [2];

        if (read(0,line,1)) // note: destructive read
        		return (int) line[0];
        
        return -1;
}

int	term_getchar() 
{ 
        int    c;

        while ((c = term_getchar_nb()) == -1)
                ;
        return c;
}

/*------------------------------------------------------------
 * Serial I/O 
 * 8 bits, 1 stopbit, no parity, 
 * 115,200 baud wired and 9,600 baud wireless.
 *------------------------------------------------------------
 */
#include <termios.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#define SERIAL_DEVICE	"/dev/ttyS0"
#define USB_DEVICE	"/dev/ttyUSB0"
#define WIFI_DEVICE 	"/dev/ttyUSB0" /* may need to change this */

int serial_device = 0;
int fd_RS232;

int rs232_open(void)
{
  	char 		*name;
  	int 		result;  
  	struct termios	tty;

	if (serial_device == 0) 
	{   
		fd_RS232 = open(SERIAL_DEVICE, O_RDWR | O_NOCTTY);
		fprintf(stderr,"using /dev/ttyS0\n"); 

	} 
	else if ( (serial_device == 1) || (serial_device == 2) ) 
	{
        	fd_RS232 = open(USB_DEVICE, O_RDWR | O_NOCTTY);
		fprintf(stderr,"using /dev/ttyUSB0\n"); 
	} 

	assert(fd_RS232>=0);

  	result = isatty(fd_RS232);
  	assert(result == 1);

  	name = ttyname(fd_RS232);
  	assert(name != 0);

  	result = tcgetattr(fd_RS232, &tty);	
	assert(result == 0);

	tty.c_iflag = IGNBRK; /* ignore break condition */
	tty.c_oflag = 0;
	tty.c_lflag = 0;

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; /* 8 bits-per-character */
	tty.c_cflag |= CLOCAL | CREAD; /* Ignore model status + read input */		

	/* Set output and input baud rates. 
	 */
	if (serial_device == 0 || serial_device == 1) // wired 
	{  
		cfsetospeed(&tty, B115200); 
		cfsetispeed(&tty, B115200); 
	} 
    	else if (serial_device == 2) // wireless 
	{  
		cfsetospeed(&tty, B9600); 
		cfsetispeed(&tty, B9600); 
	}

	tty.c_cc[VMIN]  = 0;
	tty.c_cc[VTIME] = 0;

	tty.c_iflag &= ~(IXON|IXOFF|IXANY);

	result = tcsetattr (fd_RS232, TCSANOW, &tty); /* non-canonical */

	tcflush(fd_RS232, TCIOFLUSH); /* flush I/O buffer */
}


int 	rs232_close(void)
{
  	int 	result;

  	result = close(fd_RS232);
  	assert (result==0);
}


int	rs232_getchar_nb()
{
	int 		result;
	unsigned char 	c;

	result = read(fd_RS232, &c, 1);

	if (result == 0) 
		return -1;
	
	else 
	{
		assert(result == 1);   
		return (int) c;
	}
}


int 	rs232_getchar()
{
	int 	c;

	while ((c = rs232_getchar_nb()) == -1) 
		;
	return c;
}


int 	rs232_putchar(char c)
{ 
	int result;

	do {
		result = (int) write(fd_RS232, &c, 1);
	} while (result == 0);   

	assert(result == 1);
	return result;
}


/*----------------------------------------------------------------
 * main -- execute terminal
 *----------------------------------------------------------------
 */
int main(int argc, char **argv)
{
	int 	bad_input = 0;
	char	c;
	
	/* Check input 
	 */
	if (argc == 1) 
    		serial_device = 1; 
	
	else if (argc == 2) 
	{
		if (strncmp(argv[1],"serial",3) == 0)
			serial_device = 0;

		else if (strncmp(argv[1],"usb",3) == 0)
			serial_device = 1;

		else if (strncmp(argv[1],"wifi",3) == 0)
			serial_device = 2;

		else 
			bad_input = 1;	
	} 
	else 
		bad_input = 1;

	if (bad_input == 1) 
	{
		fprintf(stderr,"Usage: ./term [serial|usb|wifi]\n"); 
		return -1;
	}

	term_puts("\nTerminal program (Embedded Software Lab), ");

	term_initio();
	rs232_open();

	term_puts("Type ^C to exit\n");

	/* discard any incoming text
	 */
	while ((c = rs232_getchar_nb()) != -1)
		fputc(c,stderr);
	
	/* send & receive
	 */
	for (;;) 
	{
		if ((c = term_getchar_nb()) != -1) 
			rs232_putchar(c);
		
		if ((c = rs232_getchar_nb()) != -1) 
			term_putchar(c);
	}

	term_exitio();
	rs232_close();
	term_puts("\n<exit>\n");
  	
	return 0;
}


