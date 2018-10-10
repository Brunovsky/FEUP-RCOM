#ifndef LL_H___
#define LL_H___

#include <stddef.h>

typedef struct {
    char a, c;
    char* data;
} frame;

int setup_link_layer(const char* name);

int reset_link_layer(int fd);

int writeFrame(int fd, frame f);

int readFrame(int fd, frame* fp);

#endif // LL_H___
