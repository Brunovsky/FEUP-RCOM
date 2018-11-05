#ifndef LL_INTERFACE_H___
#define LL_INTERFACE_H___

#include "strings.h"

#define LL_OK                  0x00
#define LL_WRONG_COMMAND       0x10
#define LL_WRONG_RESPONSE      0x11
#define LL_NO_TIME_RETRIES     0x20
#define LL_NO_ANSWER_RETRIES   0x21

#define LLOPEN_ASSUME_UA_OK    1
#define LLCLOSE_ASSUME_DISC_OK 1

int llopen(int fd);

int llclose(int fd);

int llwrite(int fd, string message);

int llread(int fd, string* messagep);

#endif // LL_INTERFACE_H___
