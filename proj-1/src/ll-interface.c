#include "ll-interface.h"
#include "ll-frames.h"
#include "options.h"
#include "debug.h"

#include <stdlib.h>
#include <stdio.h>

static int llopen_transmitter(int fd) {
    int time_count = 0;

    while (time_count < time_retries) {
        writeSETframe(fd);

        frame f;
        int s = readFrame(fd, &f);

        if (s == FRAME_READ_TIMEOUT) {
            ++time_count;
            continue;
        } else if (s == 0 && isUAframe(f)) {
            if (TRACE_LL || TRACE_FILE) printf("[LL] llopen OK\n");
            return LL_OK;
        } else {
            printf("[LL] llopen FAILED: invalid (not UA) response [s=0x%02x]\n", s);
            return LL_WRONG_RESPONSE;
        }
    }

    printf("[LL] llopen FAILED: %d time retries ran out\n", time_retries);
    return LL_NO_TIME_RETRIES;
}

static int llopen_receiver(int fd) {
    int time_count = 0;

    while (time_count < time_retries) {
        frame f;
        int s = readFrame(fd, &f);

        if (s == FRAME_READ_TIMEOUT) {
            ++time_count;
            continue;
        } else if (s == 0 && isSETframe(f)) {
            writeUAframe(fd);
            if (TRACE_LL || TRACE_FILE) printf("[LL] llopen OK\n");
            return LL_OK;
        } else {
            printf("[LL] llopen FAILED: invalid (not SET) command [s=0x%02x]\n", s);
            return LL_WRONG_COMMAND;
        }
    }

    printf("[LL] llopen FAILED: %d time retries ran out\n", time_retries);
    return LL_NO_TIME_RETRIES;
}

static int llclose_transmitter(int fd) {
    int time_count = 0;

    while (time_count < time_retries) {
        writeDISCframe(fd);

        frame f;
        int s = readFrame(fd, &f);

        if (s == FRAME_READ_TIMEOUT) {
            ++time_count;
            continue;
        } else if (s == 0 && isDISCframe(f)) {
            writeUAframe(fd);
            if (TRACE_LL || TRACE_FILE) printf("[LL] llclose OK\n");
            return LL_OK;
        } else {
            printf("[LL] llclose FAILED: invalid (not DISC) response [s=0x%02x]\n", s);
            return LL_WRONG_RESPONSE;
        }
    }

    printf("[LL] llclose FAILED: %d time retries ran out\n", time_retries);
    return LL_NO_TIME_RETRIES;
}

static int llclose_receiver(int fd) {
    int time_count = 0;

    while (time_count < time_retries) {
        frame f;
        int s = readFrame(fd, &f);

        if (s == FRAME_READ_TIMEOUT) {
            ++time_count;
            continue;
        } else if (s == 0 && isDISCframe(f)) {
            writeDISCframe(fd);

            s = readFrame(fd, &f);

            if (s == 0 && isUAframe(f)) {
                if (TRACE_LL || TRACE_FILE) printf("[LL] llclose OK\n");
                return LL_OK;
            } else {
                printf("[LL] llclose FAILED: invalid (not UA) response [s=0x%02x]\n", s);
                return LL_WRONG_RESPONSE;
            }
        } else {
            printf("[LL] llclose FAILED: invalid (not DISC) command [s=0x%02x]\n", s);
            return LL_WRONG_COMMAND;
        }
    }

    printf("[LL] llclose FAILED: %d time retries ran out\n", time_retries);
    return LL_NO_TIME_RETRIES;
}

int llopen(int fd) {
    if (my_role == TRANSMITTER) {
        return llopen_transmitter(fd);
    } else {
        return llopen_receiver(fd);
    }
}

int llwrite(int fd, string message) {
    static int index = 0; // only supports one fd.

    int time_count = 0, answer_count = 0;

    while (time_count < time_retries && answer_count < answer_retries) {
        int s = writeIframe(fd, message, index);
        if (s != FRAME_WRITE_OK) {
            ++time_count;
            continue;
        }

        frame f;
        s = readFrame(fd, &f);

        switch (s) {
        case FRAME_READ_OK:
            if (isRRframe(f, index + 1) || isREJframe(f, index + 1)) {
                ++index;
                if (TRACE_LL) {
                    printf("[LL] llwrite OK [index=%d]\n", index);
                }
                return LL_OK;
            } else if (isRRframe(f, index) || isREJframe(f, index)) {
                ++answer_count;
            } else {
                if (TRACE_LL) {
                    printf("[LL] llwrite: invalid response (not RR or REJ)\n");
                }
                ++answer_count;
            }
            break;
        case FRAME_READ_TIMEOUT:
            ++time_count;
            break;
        case FRAME_READ_INVALID:
            ++answer_count;
            break;
        }
    }

    if (time_count == time_retries) {
        printf("[LL] llwrite FAILED: %d time retries ran out\n", time_retries);
        return LL_NO_TIME_RETRIES;
    } else {
        printf("[LL] llwrite FAILED: %d answer retries ran out\n", answer_retries);
        return LL_NO_ANSWER_RETRIES;
    }
}

int llread(int fd, string* messagep) {
    static int index = 0; // only supports one fd.

    int time_count = 0, answer_count = 0;

    while (time_count < time_retries && answer_count < answer_retries) {
        frame f;
        int s = readFrame(fd, &f);

        switch (s) {
        case FRAME_READ_OK:
            if (isIframe(f, index)) {
                writeRRframe(fd, ++index);
                *messagep = f.data;
                if (TRACE_LL) {
                    printf("[LL] llread OK [index=%d]\n", index);
                }
                return LL_OK;
            } else if (isIframe(f, index + 1)) {
                writeRRframe(fd, index);
                ++answer_count;
                if (TRACE_LL) {
                    printf("[LL] llread: Expected frame %d, got frame %d\n",
                        index % 2, (index + 1) % 2);
                }
            }
            break;
        case FRAME_READ_TIMEOUT:
            ++time_count;
            break;
        case FRAME_READ_INVALID:
            writeREJframe(fd, index);
            ++answer_count;
            break;
        }
    }

    if (time_count == time_retries) {
        printf("[LL] llwrite FAILED: %d time retries ran out\n", time_retries);
        return LL_NO_TIME_RETRIES;
    } else {
        printf("[LL] llwrite FAILED: %d answer retries ran out\n", answer_retries);
        return LL_NO_ANSWER_RETRIES;
    }
}

int llclose(int fd) {
    if (my_role == TRANSMITTER) {
        return llclose_transmitter(fd);
    } else {
        return llclose_receiver(fd);
    }
}
