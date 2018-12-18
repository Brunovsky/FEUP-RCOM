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
    int port;
} url_t;

int parse_url();

int ftp_open_protocol_socket();

int ftp_login();

int ftp_open_passive_socket();

int send_retrieve();

int download_file();

#endif // SOCKETIO_H___
