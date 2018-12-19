#ifndef PIPELINE_H___
#define PIPELINE_H___

typedef struct {
    char* protocol;
    char* username;
    char* password;
    char* hostname;
    char* pathname;
    char* filename;
    int port;
} url_t;

int parse_url(const char* urlstr);

int ftp_open_control_socket();

int ftp_login();

int ftp_open_passive_socket();

int send_retrieve();

int download_file();

#endif // PIPELINE_H___
