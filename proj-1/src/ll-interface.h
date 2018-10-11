#ifndef LL_INTERFACE_H___
#define LL_INTERFACE_H___

typedef enum {
	TRANSMITTER, RECEIVER
} role_t;

int llopen(int fd, role_t role);

int llclose(int fd, role_t role);

int llwrite(int fd, char* message);

int llread(int fd, char** messagep);

#endif // LL_INTERFACE_H___
