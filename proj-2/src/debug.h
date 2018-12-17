#ifndef DEBUG_H___
#define DEBUG_H___

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#define NDEBUG

#define PRINT_PROGRESS 0

#define PRINT_FTP_COMMAND 1

#define PRINT_FTP_ECHO 1

#define PRINT_DEBUG_SOCKETIO 0

/**
 * Logging functions
 */

static void progress(const char* format, ...) {
    va_list arglist;
    va_start(arglist, format);
#if PRINT_PROGRESS
    vprintf(format, arglist);
    printf("\n");
#endif
    va_end(arglist);
}

static void ftpcommand(const char* line) {
#if PRINT_FTP_COMMAND
    printf("      [COMMD] %s", line);
#endif
}

static void ftpreply(const char* line) {
#if PRINT_FTP_ECHO
    printf("      [REPLY] %s", line);
#endif
}

static void ftpsocketio(const char* format, ...) {
    va_list arglist;
    va_start(arglist, format);
#if PRINT_DEBUG_SOCKETIO
    vprintf(format, arglist);
    printf("\n");
#endif
    va_end(arglist);
}

static void fail(const char* format, ...) {
    int err = errno;
    printf("FAIL:\n ");
    va_list arglist;
    va_start(arglist, format);
    vfprintf(stderr, format, arglist);
    printf("\n");
    va_end(arglist);
    errno = err;
    perror("Library error (errno)");
    exit(1);
}

#endif // DEBUG_H___
