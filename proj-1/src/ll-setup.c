#include "ll-setup.h"
#include "debug.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define BAUDRATE B115200

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
        perror("[SETUP] Failed to open terminal");
        exit(EXIT_FAILURE);
    }

    if (TRACE_SETUP) printf("[SETUP] Opened device %s\n", name);

    // Save current terminal settings in oldtios.
    if (tcgetattr(fd, &oldtios) == -1) {
        perror("[SETUP] Failed to read old terminal settings (tcgetattr)");
        exit(EXIT_FAILURE);
    }

    // Setup new termios
    struct termios newtio;
    memset(&newtio, 0, sizeof(struct termios));

    // c_iflag   Error handling...
    // IGNPAR :- Ignore framing errors and parity errors
    // ...
    newtio.c_iflag = IGNPAR;
    
    // c_oflag   Output delay...
    // ...
    newtio.c_oflag = 0;

    // c_cflag   Input manipulation, baud rates...
    // CS*    :- Character size mask (CS5, CS6, CS7, CS8)
    // CLOCAL :- Ignore modem control lines
    // CREAD  :- Enable receiver
    // CSTOPB :- Set two stop bits, rather than one
    // ...
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;

    // c_lflag   Canonical or non canonical mode...
    // ICANON :- Enable canonical mode
    // ECHO   :- Echo input characters
    // ...
    newtio.c_lflag = 0;

    // c_cc   Special characters
    // VTIME  :- Timeout in deciseconds for noncanonical read (TIME).
    // VMIN   :- Minimum number of characters for noncanonical read (MIN).
    // ...
    newtio.c_cc[VTIME] = 1;
    newtio.c_cc[VMIN] = 0;

    // VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    // leitura do(s) proximo(s) caracter(es)

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("[SETUP] Failed to set new terminal settings (tcsetattr)");
        exit(EXIT_FAILURE);
    }

    if (TRACE_SETUP) printf("[SETUP] Setup link layer on %s\n", name);
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
        perror("[RESET] Failed to set old terminal settings (tcsetattr)");
        close(fd);
        return 1;
    } else {
        if (TRACE_SETUP) {
            printf("[RESET] Reset device\n");
        }
        close(fd);
        return 0;
    }
}
