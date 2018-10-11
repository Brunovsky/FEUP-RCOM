#include "ll-frames.h"

#define FRAME_A_COMMAND  0x03
#define FRAME_A_RESPONSE 0x01

#define FRAME_C_SET      0x03
#define FRAME_C_DISC     0x0b
#define FRAME_C_UA       0x07
#define FRAME_C_RR(n)    ((n % 2) ? 0x85 : 0x05)
#define FRAME_C_REJ(n)   ((n % 2) ? 0x81 : 0x01)

/**
 * Frames I, SET, DISC: Command
 *
 * Frames UA, RR, REJ: Response
 */

bool isIframe(frame f, int parity) {
	return f.a == FRAME_A_COMMAND &&
	       f.c == (parity % 2) &&
	       f.data != NULL;
}

bool isSETframe(frame f) {
	return f.a == FRAME_A_COMMAND &&
	       f.c == FRAME_C_SET &&
	       f.data == NULL;
}

bool isDISCframe(frame f) {
	return f.a == FRAME_A_COMMAND &&
	       f.c == FRAME_C_DISC &&
	       f.data == NULL;
}

bool isUAframe(frame f) {
	return f.a == FRAME_A_RESPONSE &&
	       f.c == FRAME_C_UA &&
	       f.data == NULL;
}

bool isRRframe(frame f, int parity) {
	return f.a == FRAME_A_RESPONSE &&
	       f.c == FRAME_C_RR(parity) &&
	       f.data == NULL;
}

bool isREJframe(frame f, int parity) {
	return f.a == FRAME_A_RESPONSE &&
	       f.c == FRAME_C_REJ(parity) &&
	       f.data == NULL;
}

int writeIframe(int fd, char* message, int index) {
	frame f = {
		.a = FRAME_A_COMMAND,
		.c = index % 2,
		.data = message
	};

	return writeFrame(fd, f);
}

int writeSETframe(int fd) {
	frame f = {
		.a = FRAME_A_COMMAND,
		.c = FRAME_C_SET,
		.data = NULL
	};

	return writeFrame(fd, f);
}

int writeDISCframe(int fd) {
	frame f = {
		.a = FRAME_A_COMMAND,
		.c = FRAME_C_DISC,
		.data = NULL
	};

	return writeFrame(fd, f);
}

int writeUAframe(int fd) {
	frame f = {
		.a = FRAME_A_RESPONSE,
		.c = FRAME_C_UA,
		.data = NULL
	};

	return writeFrame(fd, f);
}

int writeRRframe(int fd, int parity) {
	frame f = {
		.a = FRAME_A_RESPONSE,
		.c = FRAME_C_RR(parity),
		.data = NULL
	};

	return writeFrame(fd, f);
}

int writeREJframe(int fd, int parity) {
	frame f = {
		.a = FRAME_A_RESPONSE,
		.c = FRAME_C_REJ(parity),
		.data = NULL
	};

	return writeFrame(fd, f);
}
