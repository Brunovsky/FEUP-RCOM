#ifndef LL_INTERFACE_H___
#define LL_INTERFACE_H___

typedef enum {
	TRANSMITTER, RECEIVER
} role_t;

int llopen(int fd, role_t role);

#endif // LL_INTERFACE_H___
