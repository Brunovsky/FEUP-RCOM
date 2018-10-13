#ifndef LL_INTERFACE_H___
#define LL_INTERFACE_H___

#include "strings.h"

int llopen(int fd);

int llclose(int fd);

int llwrite(int fd, string message);

int llread(int fd, string* messagep);

#endif // LL_INTERFACE_H___
