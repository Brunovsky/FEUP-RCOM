#include "ll-core.h"

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

// |  F  |  A  |  C  | BCC1 | D1 | D2

#define BAUDRATE              B38400
#define FRAME_ESC             0x7d
#define FRAME_FLAG_STUFFING   0x5e
#define FRAME_ESC_STUFFING    0x5d
#define FRAME_FLAG            0x7e

#define FRAME_READ_BAD_LENGTH 0x01
#define FRAME_READ_BAD_BCC1   0x02
#define FRAME_READ_BAD_BCC2   0x03
#define FRAME_READ_BAD_ESCAPE 0x04

typedef enum {
    READ_PRE_FRAME, READ_START_FLAG, READ_WITHIN_FRAME, READ_END_FLAG
} FrameReadState;

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

static int stuffData(char* in, char** outp, char* bcc2) {
    size_t len = strlen(in);
    size_t count = 0;

    for (size_t i = 0; i < len; ++i) {
        if (in[i] == FRAME_ESC || in[i] == FRAME_FLAG) {
            ++count;
        }
    }

    size_t out_len = len + count;
    char* buf = malloc((out_len + 1) * sizeof(char));

    char parity = 0;

    for (size_t i = 0, j = 0; i < len; ++i, ++j) {
        switch (in[i]) {
        case FRAME_FLAG:
            buf[j++] = FRAME_ESC;
            buf[j] = FRAME_FLAG_STUFFING;
            break;
        case FRAME_ESC:
            buf[j++] = FRAME_ESC;
            buf[j] = FRAME_ESC_STUFFING;
            break;
        default:
            buf[j] = in[i];
        }

        parity ^= buf[j];
    }

    buf[out_len] = '\0';

    *outp = buf;
    *bcc2 = parity;
    return 0;
}

static int destuffData(char* in, char** outp, char* bcc2) {
    size_t len = strlen(in);
    size_t count = 0;

    for (size_t i = 0; i < len; ++i) {
        if (in[i] == FRAME_ESC) {
            ++count;
        }
    }

    size_t out_len = len - count;
    char* buf = malloc((out_len + 1) * sizeof(char));

    char parity = 0;

    for (size_t i = 0, j = 0; i < len; ++i, ++j) {
        if (in[i] == FRAME_ESC) {
            switch (in[++i]) {
            case FRAME_FLAG_STUFFING:
                buf[j] = FRAME_FLAG;
                break;
            case FRAME_ESC_STUFFING:
                buf[j] = FRAME_ESC;
                break;
            default:
                free(buf);
                return FRAME_READ_BAD_ESCAPE;
            }
        } else {
            buf[j] = in[i];
        }

        parity ^= buf[j];
    }

    buf[out_len] = '\0';

    *outp = buf;
    *bcc2 = parity;
    return 0;
}

static int destuffText(char* text, char** outp, char* bcc2) {
    size_t data_len = strlen(text) - 6;

    char* tmp = malloc((data_len + 1) * sizeof(char));

    strncpy(tmp, text + 4, data_len);
    tmp[data_len] = '\0';

    int s = destuffData(tmp, outp, bcc2);

    free(tmp);
    return s;
}

static int buildText(frame f, char** textp) {
    if (f.data == NULL) {
        // S or U frame (control frame)
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
        // I frame (data frame)
        char* stuffed_data;
        char bcc2;
        stuffData(f.data, &stuffed_data, &bcc2);

        size_t text_len = strlen(stuffed_data) + 6;
        char* buf = malloc((text_len + 1) * sizeof(char));

        buf[0] = FRAME_FLAG;
        buf[1] = f.a;
        buf[2] = f.c;
        buf[3] = f.a ^ f.c;
        strncpy(buf + 4, stuffed_data, strlen(stuffed_data));
        buf[text_len - 2] = bcc2;
        buf[text_len - 1] = FRAME_FLAG;
        buf[text_len] = '\0';

        *textp = buf;
        return 0;
    }
}

static int readText(int fd, char** textp) {
    char* buf = malloc(8 * sizeof(char));
    size_t reserved = 8;
    size_t j = 0;

    FrameReadState state = READ_PRE_FRAME;

    while (state != READ_END_FLAG) {
        char readbuf[2];
        read(fd, &readbuf, 1);
        char c = readbuf[0];

        switch (state) {
        case READ_PRE_FRAME:
            if (c == FRAME_FLAG) {
                state = READ_START_FLAG;
                buf[j++] = FRAME_FLAG;
            }
            break;
        case READ_START_FLAG:
            if (c != FRAME_FLAG) {
                state = READ_WITHIN_FRAME;
                buf[j++] = c;
            }
            break;
        case READ_WITHIN_FRAME:
            if (c == FRAME_FLAG) {
                state = READ_END_FLAG;
                buf[j++] = FRAME_FLAG;
            } else {
                buf[j++] = c;
            }
            break;
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

int writeFrame(int fd, frame f) {
    char* text;
    buildText(f, &text);

    write(fd, text, strlen(text));
    free(text);
    return 0;
}

int readFrame(int fd, frame* fp) {
    char* text;
    readText(fd, &text);

    size_t text_len = strlen(text);

    if (text_len < 5 || text_len == 6) {
        free(text);
        return FRAME_READ_BAD_LENGTH;
    }

    frame f = {
        .a = text[0],
        .c = text[1],
        .data = NULL
    };

    char bcc1 = text[2];

    if (bcc1 != (f.a ^ f.c)) {
        free(text);
        return FRAME_READ_BAD_BCC1;
    }

    if (text_len > 6) {
        char* data = NULL;
        char bcc2;
        destuffText(text, &data, &bcc2);

        if (bcc2 != text[text_len - 2]) {
            free(text);
            free(data);
            return FRAME_READ_BAD_BCC2;
        }

        f.data = data;
    }

    *fp = f;

    free(text);
    return 0;
}
