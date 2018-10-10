#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>

#define BAUDRATE B38400

/**
 * Opens the terminal with given file name, changes its configuration
 * according to the specs, and returns the file descriptor fd.
 *
 * Assumption: name should be /dev/ttyS0 or /dev/ttyS1.
 *
 * @param name The terminal's name
 * @returns The terminal's file descriptor. Exits if unsuccessful.
 */
int setup_terminal(char* name) {int fd, res = 0;
    struct termios oldtio, newtio;

	// Open serial port device for reading and writing. Open as NOt Controlling TTY
	// (O_NOCTTY) because we don't want to get killed if linenoise sends CTRL-C.

    fd = open(name, O_RDWR | O_NOCTTY);
    if (fd < 0) {
    	perror(name);
    	exit(EXIT_FAILURE);
    }

    // Save current terminal settings in oldtio.
    if (tcgetattr(fd, &oldtio) == -1) {
      	perror("tcgetattr");
      	exit(EXIT_FAILURE);
    }

    // Clear the current newtio
    memset(&newtio, 0, sizeof(struct termios));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME]    = 1;
    newtio.c_cc[VMIN]     = 0;

    // VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    // leitura do(s) proximo(s) caracter(es)

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }

    fflush(NULL);
}

int sendControl(int fd) {

  char message[5];

  message[0] = 0x7E;
  message[1] = 0x03;
  message[2] = 0x03;
  message[3] = message[1]^message[2];
  message[4] = 0x7E;

  if (write(fd,message,5) < 0){
    printf("Error in writing control message at sendControl() \n");
    return -1;
  }
}
