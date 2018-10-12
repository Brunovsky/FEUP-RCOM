#ifndef LL_INTERFACE_H___
#define LL_INTERFACE_H___

#include "strings.h"

typedef enum {
	TRANSMITTER, RECEIVER
} role_t;

int llopen(int fd, role_t role);

int llclose(int fd, role_t role);

int llwrite(int fd, string message);

int llread(int fd, string* messagep);

#endif // LL_INTERFACE_H___
