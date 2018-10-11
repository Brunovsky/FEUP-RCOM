#ifndef LL_CORE_H___
#define LL_CORE_H___

#include <stddef.h>

typedef struct {
    char a, c;
    char* data;
} frame;

int writeFrame(int fd, frame f);

int readFrame(int fd, frame* fp);

#endif // LL_CORE_H___
