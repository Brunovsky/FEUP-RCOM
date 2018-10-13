#include "ll-frames.h"
#include "debug.h"

#include <stdio.h>
#include <unistd.h>

#define FRAME_A_COMMAND  0x03
#define FRAME_A_RESPONSE 0x01

#define FRAME_C_I(n)     (char)((n % 2) ? 0x01 : 0x00)
#define FRAME_C_SET      (char)0x03
#define FRAME_C_DISC     (char)0x0b
#define FRAME_C_UA       (char)0x07
#define FRAME_C_RR(n)    (char)((n % 2) ? 0x85 : 0x05)
#define FRAME_C_REJ(n)   (char)((n % 2) ? 0x81 : 0x01)

/**
 * Frames I, SET, DISC: Command
 *
 * Frames UA, RR, REJ: Response
 */

bool isIframe(frame f, int parity) {
    bool b = f.a == FRAME_A_COMMAND &&
             f.c == FRAME_C_I(parity) &&
             f.data.s != NULL && f.data.len > 0;

    if (DEBUG) printf("[LL] isIframe(%d) ? %d\n", parity % 2, (int)b);
    return b;
}

bool isSETframe(frame f) {
    bool b = f.a == FRAME_A_COMMAND &&
             f.c == FRAME_C_SET &&
             f.data.s == NULL;

    if (DEBUG) printf("[LL] isSETframe() ? %d\n", (int)b);
    return b;
}

bool isDISCframe(frame f) {
    bool b = f.a == FRAME_A_COMMAND &&
             f.c == FRAME_C_DISC &&
             f.data.s == NULL;

    if (DEBUG) printf("[LL] isDISCframe() ? %d\n", (int)b);
    return b;
}

bool isUAframe(frame f) {
    bool b = f.a == FRAME_A_RESPONSE &&
             f.c == FRAME_C_UA &&
             f.data.s == NULL;

    if (DEBUG) printf("[LL] isUAframe() ? %d\n", (int)b);
    return b;
}

bool isRRframe(frame f, int parity) {
    bool b = f.a == FRAME_A_RESPONSE &&
             f.c == FRAME_C_RR(parity) &&
             f.data.s == NULL;

    if (DEBUG) printf("[LL] isRRframe(%d) ? %d\n", parity % 2, (int)b);
    return b;
}

bool isREJframe(frame f, int parity) {
    bool b = f.a == FRAME_A_RESPONSE &&
             f.c == FRAME_C_REJ(parity) &&
             f.data.s == NULL;

    if (DEBUG) printf("[LL] isREJframe(%d) ? %d\n", parity % 2, (int)b);
    return b;
}

int writeIframe(int fd, string message, int parity) {
    frame f = {
        .a = FRAME_A_COMMAND,
        .c = FRAME_C_I(parity),
        .data = message
    };

    if (DEBUG) {
        printf("[LL] writeIframe(%d)\n", parity % 2);
        print_stringn(message);
    }
    return writeFrame(fd, f);
}

int writeSETframe(int fd) {
    frame f = {
        .a = FRAME_A_COMMAND,
        .c = FRAME_C_SET,
        .data = {NULL, 0}
    };

    if (DEBUG) printf("[LL] writeSETframe()\n");
    return writeFrame(fd, f);
}

int writeDISCframe(int fd) {
    frame f = {
        .a = FRAME_A_COMMAND,
        .c = FRAME_C_DISC,
        .data = {NULL, 0}
    };

    if (DEBUG) printf("[LL] writeDISCframe()\n");
    return writeFrame(fd, f);
}

int writeUAframe(int fd) {
    frame f = {
        .a = FRAME_A_RESPONSE,
        .c = FRAME_C_UA,
        .data = {NULL, 0}
    };

    if (DEBUG) printf("[LL] writeUAframe()\n");
    return writeFrame(fd, f);
}

int writeRRframe(int fd, int parity) {
    frame f = {
        .a = FRAME_A_RESPONSE,
        .c = FRAME_C_RR(parity),
        .data = {NULL, 0}
    };

    if (DEBUG) printf("[LL] writeRRframe(%d)\n", parity % 2);
    return writeFrame(fd, f);
}

int writeREJframe(int fd, int parity) {
    frame f = {
        .a = FRAME_A_RESPONSE,
        .c = FRAME_C_REJ(parity),
        .data = {NULL, 0}
    };

    if (DEBUG) printf("[LL] writeREJframe(%d)\n", parity % 2);
    return writeFrame(fd, f);
}
