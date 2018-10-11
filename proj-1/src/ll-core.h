#ifndef LL_CORE_H___
#define LL_CORE_H___

#include <stddef.h>

#define FRAME_READ_OK       0x00
#define FRAME_READ_TIMEOUT  0x01
#define FRAME_READ_INVALID  0x02

typedef struct {
    char a, c;
    char* data;
} frame;

int writeFrame(int fd, frame f);

int readFrame(int fd, frame* fp);

#endif // LL_CORE_H___
