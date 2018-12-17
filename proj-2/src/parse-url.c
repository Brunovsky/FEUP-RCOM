#include "parse-url.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

/**
 *
typedef struct {
    char* protocol;
    char* username;
    char* password;
    char* hostname;
    char* pathname;
    char* filename;
} url_t;
 */

static char* regexcap(char* source, regmatch_t match) {
    ssize_t start = match.rm_so;
    ssize_t end = match.rm_eo;
    size_t len = end - start;
    char* buffer = malloc((len + 1) * sizeof(char));
    strncpy(buffer, source + start, len);
    buffer[len] = '\0';
    return buffer;
}

void free_url(url_t url) {
    free(url.protocol);
    free(url.username);
    free(url.password);
    free(url.hostname);
    free(url.pathname);
    free(url.filename);
}

/**
 * Parse the given url string into a generous struct.
 * Uses a weak regular expression to parse the url.
 */
url_t parse_url(char* urlstr) {
    regex_t regex;
    //                              user   :pass   @host   /path/filename
    int s = regcomp(&regex, "^(ftp)://([^:]+):([^@]+)@([^/]+)/(.+)/([^/]+)$", REG_EXTENDED | REG_ICASE | REG_NEWLINE);
    if (s != 0) {
        char errbuf[2048];
        regerror(s, &regex, errbuf, 2048);
        fprintf(stderr, "Invalid regex: %s.\n", errbuf);
        exit(1);
    }

    regmatch_t pmatch[7];

    s = regexec(&regex, urlstr, 7, pmatch, 0);
    if (s != 0) {
        fprintf(stderr, "Invalid URL (failed regex parse).\n");
        exit(1);
    }

    url_t url = {
        .protocol = regexcap(urlstr, pmatch[1]),
        .username = regexcap(urlstr, pmatch[2]),
        .password = regexcap(urlstr, pmatch[3]),
        .hostname = regexcap(urlstr, pmatch[4]),
        .pathname = regexcap(urlstr, pmatch[5]),
        .filename = regexcap(urlstr, pmatch[6]),
    };

    fprintf(stdout, "Valid URL!\n");
    return url;
}

int main(int argc, char** argv) {
    if (argc == 1) {
        fprintf(stderr, "Please write a URL");
        return 1;
    }
    
    url_t url = parse_url(argv[1]);

    printf("protocol: %s;\n", url.protocol);
    printf("username: %s;\n", url.username);
    printf("password: %s;\n", url.password);
    printf("hostname: %s;\n", url.hostname);
    printf("pathname: %s;\n", url.pathname);
    printf("filename: %s;\n", url.filename);
    return 0;
}
