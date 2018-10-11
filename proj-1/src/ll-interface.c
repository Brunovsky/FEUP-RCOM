#include "ll-interface.h"
#include "ll-frames.h"

static size_t retries = 3;

static int llopen_transmitter(int fd) {
    writeSETframe(fd);

    frame f;
    readFrame(fd, &f);

    if (isUAframe(f)) {
        return 0;
    } else {
        return 1;
    }
}

static int llopen_receiver(int fd) {
    frame f;
    readFrame(fd, &f);

    if (isSETframe(f)) {
        writeUAframe(fd);
        return 0;
    } else {
        return 1;
    }
}

int llopen(int fd, role_t role) {
    if (role == TRANSMITTER) {
        return llopen_transmitter(fd);
    } else {
        return llopen_receiver(fd);
    }
}

static int llclose_transmitter(int fd) {
    size_t count = 3;

    do {
        writeDISCframe(fd);

        frame f;
        readFrame(fd, &f);

        if (isDISCframe(f)) {
            writeUAframe(fd);
            return 0;
        }
    } while (++count < retries);
}

static int llclose_receiver(int fd) {
    frame f;
    readFrame(fd, &f);

    if (isDISCframe(f)) {
        writeDISCframe(fd);

        readFrame(fd, &f);

        if (isUAframe(f)) {
            return 0;
        } else {
            return 1;
        }
    } else {
        return 1;
    }
}

int llclose(int fd, role_t role) {
    if (role == TRANSMITTER) {
        return llclose_transmitter(fd);
    } else {
        return llclose_receiver(fd);
    }
}

int llwrite(int fd, char* message) {
    static int index = 0; // only supports one fd.

    size_t count = 0;

    do {
        writeIframe(fd, message, index);

        frame f;
        readFrame(fd, &f);

        if (isRRframe(f, index + 1)) {
            return 0;
        } else if (!isREJframe(f, index)) {
            ++count;
        }
    } while (count < retries);

    return 1;
}

int llread(int fd, char** messagep) {
    static int index = 0; // only supports one fd.

    size_t count = 0;

    do {
        frame f;
        readFrame(fd, &f);

        if (isIframe(f, index)) {
            writeRRframe(fd, ++index);
            return 0;
        } else {
            writeREJframe(fd, index);
        }
    } while (count < retries);

    return 1;
}
