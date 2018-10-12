#ifndef APP_CORE_H___
#define APP_CORE_H___

#include <stddef.h>

int send_data_packet(int fd, string packet);

int send_start_packet(int fd, size_t filesize, const char* filename);

int send_end_packet(int fd);

#endif // APP_CORE_H___
