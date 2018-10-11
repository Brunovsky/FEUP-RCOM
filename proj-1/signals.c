#include "signals.h"
#include "options.h"
#include "log.h"
#include "debug.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

static volatile sig_atomic_t alarmed = 0;

static const char str_kill[]  = "server: Terminating...\n";
static const char str_abort[] = "server: Aborting...\n";
static const char str_pipe[]  = "server: Caught SIGPIPE...\n";
static const char str_alarm[] = "server: Alarmed...\n";

// SIGHUP, SIGQUIT, SIGTERM, SIGINT
static void sighandler_kill(int signum) {
    if (PDEBUG) write(STDOUT_FILENO, str_kill, strlen(str_kill));
    exit(EXIT_FAILURE);
}

// SIGABRT
static void sighandler_abort(int signum) {
    if (PDEBUG) write(STDOUT_FILENO, str_abort, strlen(str_abort));
    abort();
}

// SIGPIPE
static void sighandler_pipe(int signum) {
    if (PDEBUG) write(STDOUT_FILENO, str_pipe, strlen(str_pipe));
    // Ignore at the moment...
}

// SIGALRM
static void sighandler_alarm(int signum) {
    if (PDEBUG) write(STDOUT_FILENO, str_alarm, strlen(str_alarm));
    exit(EXIT_SUCCESS);
}

// TODO: fix race condition in alarm vs getline
// Fixed: call exit instead of checking alarm_timeout().

/**
 * Set the process's signal handlers and overall dispositions,
 * to be inherited by all threads.
 */
int set_signal_handlers() {
    struct sigaction action;
    sigset_t sigmask, current;

    // Get current sigmask
    int s = sigprocmask(SIG_SETMASK, NULL, &current);
    if (s != 0) {
        printf("server: Error getting process signal mask: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Set kill handler
    sigmask = current;
    action.sa_handler = sighandler_kill;
    action.sa_mask = sigmask;
    action.sa_flags = SA_RESETHAND;
    s = sigaction(SIGHUP, &action, NULL);
    if (s != 0) {
        printf("server: Error setting handler for SIGHUP: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    s = sigaction(SIGQUIT, &action, NULL);
    if (s != 0) {
        printf("server: Error setting handler for SIGQUIT: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    s = sigaction(SIGTERM, &action, NULL);
    if (s != 0) {
        printf("server: Error setting handler for SIGTERM: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    s = sigaction(SIGINT, &action, NULL);
    if (s != 0) {
        printf("server: Error setting handler for SIGINT: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Set abort handler
    sigmask = current;
    action.sa_handler = sighandler_abort;
    action.sa_mask = sigmask;
    action.sa_flags = SA_RESETHAND;
    s = sigaction(SIGABRT, &action, NULL);
    if (s != 0) {
        printf("server: Error setting handler for SIGABRT: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Set pipe handler
    sigmask = current;
    action.sa_handler = sighandler_pipe;
    action.sa_mask = sigmask;
    action.sa_flags = SA_RESTART;
    s = sigaction(SIGPIPE, &action, NULL);
    if (s != 0) {
        printf("server: Error setting handler for SIGPIPE: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Set alarm handler
    sigmask = current;
    action.sa_handler = sighandler_alarm;
    action.sa_mask = sigmask;
    action.sa_flags = SA_RESETHAND;
    s = sigaction(SIGALRM, &action, NULL);
    if (s != 0) {
        printf("server: Error setting handler for SIGALRM: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return 0;
}

void set_alarm() {
    alarm(o_time);
}

int alarm_timeout() {
    return (int)alarmed;
}
