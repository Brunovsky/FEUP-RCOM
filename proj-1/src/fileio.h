#ifndef FILEIO_H___
#define FILEIO_H___

#include "app-layer.h"

int send_file(int fd, char* filename);

int receive_file(int fd);

#endif // FILEIO_H___
