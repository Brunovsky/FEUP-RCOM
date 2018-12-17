#ifndef PARSE_URL_H___
#define PARSE_URL_H___

typedef struct {
    char* protocol;
    char* username;
    char* password;
    char* hostname;
    char* pathname;
    char* filename;
} url_t;

void free_url(url_t url);

url_t parse_url(char* urlstr);

#endif // PARSE_URL_H___
