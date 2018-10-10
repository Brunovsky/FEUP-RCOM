#include "ll-interface.h"

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
	switch (role) {
	case TRANSMITTER:
		llopen_transmitter(fd);
		break;
	case RECEIVER:
		llopen_receiver(fd);
		break;
	}
}

static int llclose_transmitter(int fd) {
	writeDISCframe(fd);

	frame f;
	readFrame(fd, &f);

	if (!isDISCframe(f)) {
		writeUAframe(fd);
		return 0;
	}
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
	switch (role) {
	case TRANSMITTER:
		llclose_transmitter(fd);
		break;
	case RECEIVER:
		llclose_receiver(fd);
		break;
	}
}
