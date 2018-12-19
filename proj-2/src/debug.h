#ifndef DEBUG_H___
#define DEBUG_H___

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/**
 * Linux terminal colors
 */
#define CRED "\e[1;31m"

#define CGREEN "\e[1;32m"

#define CYELLOW "\e[1;33m"

#define CBLUE "\e[1;34m"

#define CPURP "\e[1;35m"

#define CEND "\e[m"

/**
 * Print what?
 */
#define PRINT_PROGRESS 1

#define PRINT_FTP_COMMAND 1

#define PRINT_FTP_REPLY 1

/**
 * Logging functions
 */
void progress(const char* format, ...);

void ftpcommand(const char* line);

void ftpreply(const char* line);

void fail(const char* format, ...);

void libfail(const char* format, ...);

void unexpected(const char* format, ...);

#endif // DEBUG_H___
