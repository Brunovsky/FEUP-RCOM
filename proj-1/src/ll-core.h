#ifndef LL_CORE_H___
#define LL_CORE_H___

#include "strings.h"

#define FRAME_ESC             0x7d
#define FRAME_FLAG_STUFFING   0x5e
#define FRAME_ESC_STUFFING    0x5d
#define FRAME_FLAG            0x7e

#define FRAME_READ_OK         0x00
#define FRAME_READ_TIMEOUT    0x01
#define FRAME_READ_INVALID    0x10

#define FRAME_READ_BAD_LENGTH 0x11
#define FRAME_READ_BAD_BCC1   0x12
#define FRAME_READ_BAD_BCC2   0x13
#define FRAME_READ_BAD_ESCAPE 0x14

typedef struct {
    char a, c;
    string data;
} frame;

int writeFrame(int fd, frame f);

int readFrame(int fd, frame* fp);

#endif // LL_CORE_H___
