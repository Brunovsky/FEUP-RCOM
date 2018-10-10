#include "link-layer.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#include <assert.h>

#define BAUDRATE            B38400
#define FRAME_ESC           0x7d
#define FRAME_FLAG_STUFFING 0x5e
#define FRAME_ESC_STUFFING  0x5d
#define FRAME_FLAG          0x7e

#define FRAME_READ_BAD_LENGTH 0x01
#define FRAME_READ_BAD_BCC1   0x02
#define FRAME_READ_BAD_BCC2   0x03

typedef enum {
    PRE_FRAME, START_FLAG, WITHIN_FRAME, END_FLAG
} FrameReadState ;

static const char* terminal_name;
static struct termios oldtios;
static int fd = -1;

/**
 * Opens the terminal with given file name, changes its configuration
 * according to the specs, and returns the file descriptor fd.
 *
 * Assumption: name should be /dev/ttyS0 or /dev/ttyS1.
 *
 * @param name The terminal's name
 * @returns The terminal's file descriptor, or -1 if unsuccessful.
 */
int setup_link_layer(const char* name) {
    terminal_name = name;

	// Open serial port device for reading and writing. Open as NOt Controlling TTY
	// (O_NOCTTY) because we don't want to get killed if linenoise sends CTRL-C.

    fd = open(name, O_RDWR | O_NOCTTY);
    if (fd < 0) {
    	perror("Failed to open terminal");
    	exit(EXIT_FAILURE);
    }

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

    return fd;
}

int buildText(frame f, char** textp) {
    if (f.data == NULL) {
        char* buf = malloc(6 * sizeof(char));

        buf[0] = FRAME_FLAG;
        buf[1] = f.a;
        buf[2] = f.c;
        buf[3] = f.a ^ f.c;
        buf[4] = FRAME_FLAG;
        buf[5] = '\0';

        *textp = buf;
        return 0;
    } else {
        size_t count = 0;
        for (size_t i = 0; i < f.length; ++i) {
            if (f.data[i] == FRAME_FLAG || f.data[i] == FRAME_ESC) {
                ++count;
            }
        }

        size_t len = 7 + f.length + count;
        char* buf = malloc(len * sizeof(char));

        size_t j = 0;

        buf[j++] = FRAME_FLAG;
        buf[j++] = f.a;
        buf[j++] = f.c;
        buf[j++] = f.a ^ f.c;

        char bcc2 = 0;

        for (size_t i = 0; i < f.length; ++i) {
            switch (f.data[i]) {
            case FRAME_FLAG:
                buf[j++] = FRAME_ESC;
                buf[j++] = FRAME_FLAG_STUFFING;
                break;
            case FRAME_ESC:
                buf[j++] = FRAME_ESC;
                buf[j++] = FRAME_ESC_STUFFING;
                break;
            default:
                buf[j++] = f.data[i];
            }

            bcc2 ^= buf[j - 1];
            printf("%x\n", (int)bcc2);
        }

        buf[j++] = bcc2;
        buf[j++] = FRAME_FLAG;
        buf[j++] = '\0';

        printf("len = %lu; j = %lu, bcc2 = %d\n", len, j, (int)bcc2);
        assert(len == j);

        *textp = buf;
        return 0;
    }
}

int destuffText(char* in, char** outp) {
    size_t len = strlen(in);
    size_t count = 0;

    for (size_t i = 0; i < len; ++i) {
        if (in[i] == FRAME_ESC) {
            ++count;
        }
    }

    char* buf = malloc((len - count + 1) * sizeof(char));

    for (size_t i = 0, j = 0; i < len; ++i, ++j) {
        if (in[i] == FRAME_ESC) {
            switch (in[++i]) {
            case FRAME_FLAG_STUFFING:
                buf[j] = FRAME_FLAG;
                break;
            case FRAME_ESC_STUFFING:
                buf[j] = FRAME_ESC;
                break;
            }
        } else {
            buf[j] = in[i];
        }
    }

    buf[len - count] = '\0';

    *outp = buf;
    return 0;
}

int writeFrame(int fd, frame f) {
    char* text;
    buildText(f, &text);

    write(fd, text, strlen(text));
    free(text);
    return 0;
}

int readText(int fd, char** textp) {
    char* buf = malloc(8 * sizeof(char));
    size_t reserved = 8;
    size_t j = 0;

    FrameReadState state = PRE_FRAME;

    while (state != END_FLAG) {
        char readbuf[2];
        read(fd, &readbuf, 1);
        char c = readbuf[0];

        switch (state) {
        case PRE_FRAME:
            if (c == FRAME_FLAG) {
                state = START_FLAG;
                buf[j++] = FRAME_FLAG;
            }
            break;
        case START_FLAG:
            if (c != FRAME_FLAG) {
                state = WITHIN_FRAME;
                buf[j++] = c;
            }
            break;
        case WITHIN_FRAME:
            if (c == FRAME_FLAG) {
                state = END_FLAG;
                buf[j++] = FRAME_FLAG;
            } else {
                buf[j++] = c;
            }
            break;
        default:
            assert(false);
        }

        if ((j + 1) == reserved) {
            buf = realloc(buf, reserved * 2);
            reserved *= 2;
        }
    }

    buf[j++] = '\0';

    *textp = buf;

    return 0;
}

int readFrame(int fd, frame* fp) {
    char* text;
    readText(fd, &text);

    printf("%s", text);

    size_t len = strlen(text);
    size_t data_len = len - 6;

    if (len < 5 || len == 6) {
        return FRAME_READ_BAD_LENGTH;
    }

    frame f;

    size_t j = 1;

    f.a = text[j++];
    f.c = text[j++];

    if (text[j++] != (f.a ^ f.c)) {
        return FRAME_READ_BAD_BCC1;
    }

    char bcc2 = 0;

    while (j < len - 2) {
        bcc2 ^= text[j++];
    }

    if (bcc2 != text[j++]) {
        return FRAME_READ_BAD_BCC2;
    }

    f.data = malloc((data_len + 1) * sizeof(char));
    f.reserved = data_len + 1;
    f.length = data_len;

    strncpy(f.data, text + 4, data_len);
    f.data[data_len] = '\0';

    *fp = f;

    free(text);
    return 0;
}

int main(int argc, char** argv) {
    const char* tty = "/dev/ttyS0";

    int fd = setup_link_layer(tty);

    if (fd < 0) {
        printf("Ardeu\n");
        return 1;
    }

    frame f;

    readFrame(fd, &f);

    return 0;
}
