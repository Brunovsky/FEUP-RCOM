#include "signals.h"
#include "options.h"
#include "debug.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#define UALARM_TEST_SET    2500
#define UALARM_TEST_WAIT   500
#define UALARM_TEST_SLEEP  5500

static const char str_kill[]  = " -- Terminating...\n";
static const char str_abort[] = " -- Aborting...\n";
static const char str_alarm[] = " -- Alarmed...\n";

// SIGHUP, SIGQUIT, SIGTERM, SIGINT
static void sighandler_kill(int signum) {
    if (DEBUG) write(STDOUT_FILENO, str_kill, strlen(str_kill));
    exit(EXIT_FAILURE);
}

// SIGABRT
static void sighandler_abort(int signum) {
    if (DEBUG) write(STDOUT_FILENO, str_abort, strlen(str_abort));
    abort();
}

// SIGALRM
static void sighandler_alarm(int signum) {
    if (DEBUG) write(STDOUT_FILENO, str_alarm, strlen(str_alarm));
}

/**
 * Set the process's signal handlers and overall dispositions
 */
int set_signal_handlers() {
    struct sigaction action;
    sigset_t sigmask, current;

    // Get current sigmask
    int s = sigprocmask(SIG_SETMASK, NULL, &current);
    if (s != 0) {
        printf("[SIG] Error getting process signal mask: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Set kill handler
    sigmask = current;
    action.sa_handler = sighandler_kill;
    action.sa_mask = sigmask;
    action.sa_flags = SA_RESETHAND;
    s = sigaction(SIGHUP, &action, NULL);
    if (s != 0) {
        printf("[SIG] Error setting handler for SIGHUP: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    s = sigaction(SIGQUIT, &action, NULL);
    if (s != 0) {
        printf("[SIG] Error setting handler for SIGQUIT: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    s = sigaction(SIGTERM, &action, NULL);
    if (s != 0) {
        printf("[SIG] Error setting handler for SIGTERM: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    s = sigaction(SIGINT, &action, NULL);
    if (s != 0) {
        printf("[SIG] Error setting handler for SIGINT: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Set abort handler
    sigmask = current;
    action.sa_handler = sighandler_abort;
    action.sa_mask = sigmask;
    action.sa_flags = SA_RESETHAND;
    s = sigaction(SIGABRT, &action, NULL);
    if (s != 0) {
        printf("[SIG] Error setting handler for SIGABRT: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Set alarm handler
    sigmask = current;
    action.sa_handler = sighandler_alarm;
    action.sa_mask = sigmask;
    action.sa_flags = 0;
    s = sigaction(SIGALRM, &action, NULL);
    if (s != 0) {
        printf("[SIG] Error setting handler for SIGALRM: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (DEBUG) printf("[SIG] Set all signal handlers\n");
    return 0;
}

void set_alarm(int microseconds) {
    ualarm((useconds_t)microseconds, 0);
}

void unset_alarm() {
    ualarm(0, 0);
}

void test_alarm() {
    int s;

    set_alarm(UALARM_TEST_SET);
    
    s = usleep(UALARM_TEST_WAIT);
    if (s != 0) {
        printf("[ALARM] Failed test_alarm() -- wait interrupted\n");
        exit(EXIT_FAILURE);
    }

    unset_alarm();

    s = usleep(UALARM_TEST_SLEEP);
    if (s != 0) {
        printf("[ALARM] Failed test_alarm() -- sleep interrupted\n");
        exit(EXIT_FAILURE);
    }

    unset_alarm();

    if (DEBUG) printf("[ALARM] Passed test_alarm()\n");
}
