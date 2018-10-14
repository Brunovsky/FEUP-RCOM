#include "ll-frames.h"
#include "debug.h"

#include <stdio.h>
#include <unistd.h>

bool isIframe(frame f, int parity) {
    bool b = f.a == FRAME_A_COMMAND &&
             f.c == FRAME_C_I(parity) &&
             f.data.s != NULL && f.data.len > 0;

    if (TRACE_LL) printf("[LL] isIframe(%d) ? %d\n", parity % 2, (int)b);
    return b;
}

bool isSETframe(frame f) {
    bool b = f.a == FRAME_A_COMMAND &&
             f.c == FRAME_C_SET &&
             f.data.s == NULL;

    if (TRACE_LL) printf("[LL] isSETframe() ? %d\n", (int)b);
    return b;
}

bool isDISCframe(frame f) {
    bool b = f.a == FRAME_A_COMMAND &&
             f.c == FRAME_C_DISC &&
             f.data.s == NULL;

    if (TRACE_LL) printf("[LL] isDISCframe() ? %d\n", (int)b);
    return b;
}

bool isUAframe(frame f) {
    bool b = f.a == FRAME_A_RESPONSE &&
             f.c == FRAME_C_UA &&
             f.data.s == NULL;

    if (TRACE_LL) printf("[LL] isUAframe() ? %d\n", (int)b);
    return b;
}

bool isRRframe(frame f, int parity) {
    bool b = f.a == FRAME_A_RESPONSE &&
             f.c == FRAME_C_RR(parity) &&
             f.data.s == NULL;

    if (TRACE_LL) printf("[LL] isRRframe(%d) ? %d\n", parity % 2, (int)b);
    return b;
}

bool isREJframe(frame f, int parity) {
    bool b = f.a == FRAME_A_RESPONSE &&
             f.c == FRAME_C_REJ(parity) &&
             f.data.s == NULL;

    if (TRACE_LL) printf("[LL] isREJframe(%d) ? %d\n", parity % 2, (int)b);
    return b;
}

int writeIframe(int fd, string message, int parity) {
    frame f = {
        .a = FRAME_A_COMMAND,
        .c = FRAME_C_I(parity),
        .data = message
    };

    if (TRACE_LL) {
        printf("[LL] writeIframe(%d) [flen=%lu]\n", parity % 2, f.data.len);
        if (TEXT_DEBUG) print_stringn(message);
    }
    return writeFrame(fd, f);
}

int writeSETframe(int fd) {
    frame f = {
        .a = FRAME_A_COMMAND,
        .c = FRAME_C_SET,
        .data = {NULL, 0}
    };

    if (TRACE_LL) printf("[LL] writeSETframe()\n");
    return writeFrame(fd, f);
}

int writeDISCframe(int fd) {
    frame f = {
        .a = FRAME_A_COMMAND,
        .c = FRAME_C_DISC,
        .data = {NULL, 0}
    };

    if (TRACE_LL) printf("[LL] writeDISCframe()\n");
    return writeFrame(fd, f);
}

int writeUAframe(int fd) {
    frame f = {
        .a = FRAME_A_RESPONSE,
        .c = FRAME_C_UA,
        .data = {NULL, 0}
    };

    if (TRACE_LL) printf("[LL] writeUAframe()\n");
    return writeFrame(fd, f);
}

int writeRRframe(int fd, int parity) {
    frame f = {
        .a = FRAME_A_RESPONSE,
        .c = FRAME_C_RR(parity),
        .data = {NULL, 0}
    };

    if (TRACE_LL) printf("[LL] writeRRframe(%d)\n", parity % 2);
    return writeFrame(fd, f);
}

int writeREJframe(int fd, int parity) {
    frame f = {
        .a = FRAME_A_RESPONSE,
        .c = FRAME_C_REJ(parity),
        .data = {NULL, 0}
    };

    if (TRACE_LL) printf("[LL] writeREJframe(%d)\n", parity % 2);
    return writeFrame(fd, f);
}
