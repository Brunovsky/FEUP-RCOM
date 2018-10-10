#ifndef LL_H___
#define LL_H___

#include <stddef.h>

typedef struct {
    char a, c;

    char* data;
    size_t reserved;
    size_t length;
} frame;

#endif // LL_H___
