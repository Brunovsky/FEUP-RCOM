#include "debug.h"

void progress(const char* format, ...) {
    va_list arglist;
    va_start(arglist, format);
#if PRINT_PROGRESS
    vprintf(format, arglist);
    printf("\n");
#endif
    va_end(arglist);
}

void logftpcommand(const char* line) {
#if PRINT_FTP_COMMAND
    printf(CBLUE"    [COMMD] %s"CEND, line);
#endif
}

void logftpreply(const char* line) {
#if PRINT_FTP_REPLY
    printf(CPURP"    [REPLY] %s"CEND, line);
#endif
}

void fail(const char* format, ...) {
    printf(CRED"ERROR:\n ");
    va_list arglist;
    va_start(arglist, format);
    vfprintf(stderr, format, arglist);
    va_end(arglist);
    printf("\n"CEND);
    exit(EXIT_FAILURE);
}

void libfail(const char* format, ...) {
    int err = errno;
    printf(CRED"FAIL:\n ");
    va_list arglist;
    va_start(arglist, format);
    vfprintf(stderr, format, arglist);
    va_end(arglist);
    printf("\n");
    errno = err;
    perror("Library error (errno)");
    printf(CEND);
    exit(EXIT_FAILURE);
}

void unexpected(const char* format, ...) {
    printf(CRED"UNEXPECTED SERVER RESPONSE:\n ");
    va_list arglist;
    va_start(arglist, format);
    vfprintf(stderr, format, arglist);
    va_end(arglist);
    printf("\n"CEND);
    exit(EXIT_FAILURE);
}
