#include "ll-interface.h"
#include "ll-frames.h"
#include "options.h"

#include <stdlib.h>

static int llopen_transmitter(int fd) {
    int time_count = 0;

    while (time_count < time_retries) {
        writeSETframe(fd);

        frame f;
        int s = readFrame(fd, &f);

        if (s == FRAME_READ_TIMEOUT) {
            ++time_count;
            continue;
        } else if (isUAframe(f)) {
            return 0;
        } else {
            return 1;
        }
    }

    return 2;
}

static int llopen_receiver(int fd) {
    int time_count = 0;

    while (time_count < time_retries) {
        frame f;
        int s = readFrame(fd, &f);

        if (s == FRAME_READ_TIMEOUT) {
            ++time_count;
            continue;
        } else if (isSETframe(f)) {
            writeUAframe(fd);
            return 0;
        } else {
            return 1;
        }
    }

    return 2;
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
        } else if (isDISCframe(f)) {
            writeUAframe(fd);
            return 0;
        } else {
            return 1;
        }
    }

    return 2;
}

static int llclose_receiver(int fd) {
    int time_count = 0;

    while (time_count < time_retries) {
        frame f;
        int s = readFrame(fd, &f);

        if (s == FRAME_READ_TIMEOUT) {
            ++time_count;
            continue;
        } else if (isDISCframe(f)) {
            writeDISCframe(fd);

            readFrame(fd, &f);

            if (isUAframe(f)) {
                return 0;
            } else {
                return 3;
            }
        } else {
            return 1;
        }
    }

    return 2;
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
        writeIframe(fd, message, index);

        frame f;
        int s = readFrame(fd, &f);

        switch (s) {
        case FRAME_READ_OK:
            if (isRRframe(f, index + 1) || isREJframe(f, index + 1)) {
                ++index;
                return 0;
            } else if (isRRframe(f, index) || isREJframe(f, index)) {
                ++answer_count;
            } else {
                // Incoherent behaviour
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

    return 1;
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
                return 0;
            } else if (isIframe(f, index + 1)) {
                writeRRframe(fd, index);
                ++answer_count;
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

    return 1;
}

int llclose(int fd) {
    if (my_role == TRANSMITTER) {
        return llclose_transmitter(fd);
    } else {
        return llclose_receiver(fd);
    }
}
