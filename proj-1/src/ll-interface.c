#include "ll-interface.h"
#include "ll-frames.h"
#include "options.h"
#include "debug.h"

#include <stdlib.h>
#include <stdio.h>

static int llopen_transmitter(int fd) {
    int time_count = 0, answer_count = 0;

    while (time_count < time_retries && answer_count < answer_retries) {
        int s = writeSETframe(fd);
        if (s != FRAME_WRITE_OK) {
            ++time_count;
            continue;
        }

        frame f;
        s = readFrame(fd, &f);

        if (s == FRAME_READ_TIMEOUT) {
            ++time_count;
        } else if (s == 0 && isUAframe(f)) {
            if (TRACE_LL || TRACE_FILE) printf("[LL] llopen (T) OK\n");
            return LL_OK;
        } else {
            if (LL_ASSUME_UA_OK) {
                if (TRACE_LL || TRACE_FILE) printf("[LL] llopen (T) ASSUME UA OK\n");
                return LL_OK;
            } else {
                ++answer_count;
            }
        }
    }

    if (time_count == time_retries) {
        printf("[LL] llopen (T) FAILED: %d time retries ran out\n", time_retries);
        return LL_NO_TIME_RETRIES;
    } else {
        printf("[LL] llopen (T) FAILED: %d answer retries ran out\n", answer_retries);
        return LL_NO_ANSWER_RETRIES;
    }
}

static int llopen_receiver(int fd) {
    int time_count = 0, answer_count = 0;

    while (time_count < time_retries && answer_count < answer_retries) {
        frame f;
        int s = readFrame(fd, &f);

        if (s == FRAME_READ_TIMEOUT) {
            ++time_count;
        } else if (s == 0 && isSETframe(f)) {
            writeUAframe(fd);
            if (TRACE_LL || TRACE_FILE) printf("[LL] llopen (R) OK\n");
            return LL_OK;
        } else {
            ++answer_count;
        }
    }

    if (time_count == time_retries) {
        printf("[LL] llopen (R) FAILED: %d time retries ran out\n", time_retries);
        return LL_NO_TIME_RETRIES;
    } else {
        printf("[LL] llopen (R) FAILED: %d answer retries ran out\n", answer_retries);
        return LL_NO_ANSWER_RETRIES;
    }
}

static int llclose_transmitter(int fd) {
    int time_count = 0, answer_count = 0;

    while (time_count < time_retries && answer_count < answer_retries) {
        int s = writeDISCframe(fd);
        if (s != FRAME_WRITE_OK) {
            ++time_count;
            continue;
        }

        frame f;
        s = readFrame(fd, &f);

        if (s == FRAME_READ_TIMEOUT) {
            ++time_count;
        } else if (s == 0 && isDISCframe(f)) {
            writeUAframe(fd);
            if (TRACE_LL || TRACE_FILE) printf("[LL] llclose (T) OK\n");
            return LL_OK;
        } else {
            ++answer_count;
        }
    }

    if (time_count == time_retries) {
        printf("[LL] llclose (T) FAILED: %d time retries ran out\n", time_retries);
        return LL_NO_TIME_RETRIES;
    } else {
        printf("[LL] llclose (T) FAILED: %d answer retries ran out\n", answer_retries);
        return LL_NO_ANSWER_RETRIES;
    }
}

static int llclose_receiver(int fd) {
    int time_count = 0, answer_count = 0;

    while (time_count < time_retries && answer_count < answer_retries) {
        frame f;
        int s = readFrame(fd, &f);

        if (s == FRAME_READ_TIMEOUT) {
            ++time_count;
        } else if (s == 0 && isDISCframe(f)) {
            s = writeDISCframe(fd);
            if (s != FRAME_WRITE_OK) {
                ++time_count;
                continue;
            }

            s = readFrame(fd, &f);

            if (s == FRAME_READ_TIMEOUT) {
                ++time_count;
            } if (s == 0 && isUAframe(f)) {
                if (TRACE_LL || TRACE_FILE) printf("[LL] llclose (R) OK\n");
                return LL_OK;
            } else {
                if (LL_ASSUME_UA_OK) {
                    if (TRACE_LL || TRACE_FILE) printf("[LL] llopen (T) ASSUME UA OK\n");
                    return LL_OK;
                } else {
                    ++answer_count;
                }
            }
        } else {
            ++answer_count;
        }
    }

    if (time_count == time_retries) {
        printf("[LL] llclose (R) FAILED: %d time retries ran out\n", time_retries);
        return LL_NO_TIME_RETRIES;
    } else {
        printf("[LL] llclose (R) FAILED: %d answer retries ran out\n", answer_retries);
        return LL_NO_ANSWER_RETRIES;
    }
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
