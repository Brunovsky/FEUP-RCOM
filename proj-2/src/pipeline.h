#ifndef SOCKETIO_H___
#define SOCKETIO_H___

#include "debug.h"

typedef struct {
    char* protocol;
    char* username;
    char* password;
    char* hostname;
    char* pathname;
    char* filename;
} url_t;

url_t parse_url(char* urlstr);

int ftp_open_protocol_socket(const char* hostname);

int ftp_login(const char* username, const char* password);

int ftp_open_passive_socket();

int send_retrieve(const char* pathname, const char* filename);

int download_file(const char* filename);

#endif // SOCKETIO_H___
