/*------------------------------------------------------------
 * Simple terminal in C
 * 
 * (c) 2005/2006, Arjan J.C. van Gemund, Mark Dufour
 *------------------------------------------------------------
 */
#define	FALSE		0
#define	TRUE		1

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <stdlib.h>

struct pollfd fds[1];

/*------------------------------------------------------------
 * console I/O
 *------------------------------------------------------------
 */
struct termios 	savetty;

void	term_initio()
{
	struct termios tty;

/*        tcgetattr(0, &savetty);
        tcgetattr(0, &tty);
        tty.c_lflag &= ~(ECHO|ECHONL|ICANON|IEXTEN);
        tty.c_cc[VTIME] = 0;
        tty.c_cc[VMIN] = 0;
        tcsetattr(0, TCSADRAIN, &tty); */

		/* backup console */		
		tcgetattr(0, &savetty);
		tcgetattr(0, &tty);
		/* set console flags */
		//tty.c_lflag &= ~(ECHO|ECHONL|ICANON);
		tty.c_lflag &= ~(ECHO|ECHONL|ICANON|IEXTEN);
		//tty.c_lflag &= ~(ECHO|ECHONL|IEXTEN);
		//tty.c_lflag &= ~ICANON;
		tty.c_cc[VTIME] = 0;
		tty.c_cc[VMIN] = 0;
		tcsetattr(0, TCSADRAIN, &tty);
		
		fds[0].fd = 0;
		fds[0].events = POLLIN|POLLHUP;
}

void	term_exitio()
{
	tcsetattr(0, TCSADRAIN, &savetty);
}

void	term_puts(char *s) 
{ 
	//fprintf(stderr,"%s",s); 
	printf("%s", s);
        fflush(stdout);
}

void	term_putchar(char c) 
{ 
	//putc(c,stderr); 
	putc(c, stdout);
        fflush(stdout);
}

int	term_getchar_nb() 
{ 
		static char     foo;

		/* note: destructive read */


		if( poll(fds, 1, 0) > 0) {
		    read(0, &foo, 1);
		    //printf("gotchar %d\n", foo);
	//		fflush(stdout);
                    return foo;
                }

/*        static unsigned char 	line [2];

	// note: destructive read

        if (read(0,line,1)) {
                return (int) line[0];
        } */
        return -1;
}

int	term_getchar() 
{ 
        int    c;

        while ((c = term_getchar_nb()) == -1)
                ;
        return c;
}

#include <time.h>
#include <assert.h>
void	term_delay(unsigned int us)
{
	struct timespec	req, rem;

	req.tv_sec = 0;
	req.tv_nsec = 1000 * us;
	assert(nanosleep(&req,&rem) == 0);
}


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

//#define SERIAL_DEVICE 	"/dev/ttyS0"


int fd_RS232;


int rs232_open(void)
{
  	char 		*name;
  	int 		result;  
        struct termios  tty;

    char *serial_device = getenv("X32_PACKAGE_SERIAL");
    assert(serial_device!=0);

  	fd_RS232 = open(serial_device, O_RDWR | O_NOCTTY);
  	assert(fd_RS232>=0);

  	tcgetattr(fd_RS232, &tty);

        tty.c_iflag = IGNBRK;
        tty.c_oflag = 0;
        tty.c_lflag = 0;

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
	tty.c_cflag |= CLOCAL | CREAD;

  	cfsetospeed(&tty, B115200);
  	cfsetispeed(&tty, B115200);

  	tty.c_cc[VMIN]  = 0;
  	tty.c_cc[VTIME] = 0;
	
        tty.c_iflag &= ~(IXON|IXOFF|IXANY);

  	result = tcsetattr (fd_RS232, TCSANOW, &tty);

  	tcflush(fd_RS232, TCIOFLUSH);

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
	if (result == 0) {
		return -1;
	}
	else {
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
	char	c;

	term_initio();
	rs232_open();

	term_puts("Terminal program (c) Arjan J.C. van Gemund, Mark Dufour\n");
	term_puts("Type ^C to exit\n");

	/* discard any incoming text
	 */
	/*while ((c = rs232_getchar_nb()) != -1) 
		//fputc(c,stderr);
		term_putchar('x'); */

    if(argc > 1) {
        rs232_putchar('s');
        rs232_putchar('\n');

    } 

	for (;;) {

                if ((fds[0].revents & POLLHUP) != 0) {
			printf("hang up!\n");
			fflush(stdout);
			break;
		}
		if ((c = term_getchar_nb()) != -1) {
			rs232_putchar(c);
		}
		if ((c = rs232_getchar_nb()) != -1) {
		        term_putchar(c);
		} 
	}
	term_exitio();
	rs232_close();
	term_puts("\n<exit>\n");
  	//exit(0);
}

