#include "ll-setup.h"
#include "debug.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define BAUDRATE B38400

static struct termios oldtios;

/**
 * Opens the terminal with given file name, changes its configuration
 * according to the specs, and returns the file descriptor fd.
 *
 * Assumption: name should be /dev/ttyS0 or /dev/ttyS1.
 *
 * @param  name The terminal's name
 * @return The terminal's file descriptor, or -1 if unsuccessful
 */
int setup_link_layer(const char* name) {
    // Open serial port device for reading and writing. Open as NOt Controlling TTY
    // (O_NOCTTY) because we don't want to get killed if linenoise sends CTRL-C.

    int fd = open(name, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("Failed to open terminal");
        exit(EXIT_FAILURE);
    }

    printf("Past open\n");

    // Save current terminal settings in oldtios.
    if (tcgetattr(fd, &oldtios) == -1) {
        perror("Failed to read old terminal settings (tcgetattr)");
        exit(EXIT_FAILURE);
    }

    // Setup new termios
    struct termios newtio;
    memset(&newtio, 0, sizeof(struct termios));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 1;
    newtio.c_cc[VMIN] = 0;

    // VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    // leitura do(s) proximo(s) caracter(es)

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("Failed to set new terminal settings (tcsetattr)");
        exit(EXIT_FAILURE);
    }

    if (DEBUG) printf("Setup link layer on %s\n", name);
    return fd;
}

/**
 * Resets the terminal's settings to the old ones.
 * 
 * @param  fd The terminal's open file descriptor
 * @return 0 if successful, 1 otherwise.
 */
int reset_link_layer(int fd) {
    if (tcsetattr(fd, TCSANOW, &oldtios) == -1) {
        perror("Failed to set old terminal settings (tcsetattr)");
        close(fd);
        return 1;
    } else {
        close(fd);
        return 0;
    }
}
