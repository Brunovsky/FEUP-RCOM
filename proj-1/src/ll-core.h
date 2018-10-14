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

#define FRAME_A_COMMAND       0x03
#define FRAME_A_RESPONSE      0x01
#define FRAME_VALID_A(c)      ((c == FRAME_A_COMMAND) || (c == FRAME_A_RESPONSE))

#define FRAME_C_I(n)          (char)((n % 2) ? 0x01 : 0x00)
#define FRAME_C_SET           (char)0x03
#define FRAME_C_DISC          (char)0x0b
#define FRAME_C_UA            (char)0x07
#define FRAME_C_RR(n)         (char)((n % 2) ? 0x85 : 0x05)
#define FRAME_C_REJ(n)        (char)((n % 2) ? 0x81 : 0x01)

typedef struct {
    char a, c;
    string data;
} frame;

int writeFrame(int fd, frame f);

int readFrame(int fd, frame* fp);

#endif // LL_CORE_H___
