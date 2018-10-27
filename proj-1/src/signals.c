#include "signals.h"
#include "options.h"
#include "debug.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/time.h>

#define ALARM_TEST_SET           2500
#define ALARM_TEST_SHORT_SLEEP   500
#define ALARM_TEST_LONG_SLEEP    8500

static const char str_kill[]  = "[SIG] -- Terminating...\n";
static const char str_abort[] = "[SIG] -- Aborting...\n";
static const char str_alarm[] = "[SIG] -- Alarmed...\n";

static volatile sig_atomic_t alarmed = 0;

// SIGHUP, SIGQUIT, SIGTERM, SIGINT
static void sighandler_kill(int signum) {
    if (TRACE_SIG) write(STDOUT_FILENO, str_kill, strlen(str_kill));
    exit(EXIT_FAILURE);
}

// SIGABRT
static void sighandler_abort(int signum) {
    if (TRACE_SIG) write(STDOUT_FILENO, str_abort, strlen(str_abort));
    abort();
}

// SIGALRM
static void sighandler_alarm(int signum) {
    if (TRACE_SIG) write(STDOUT_FILENO, str_alarm, strlen(str_alarm));
    alarmed = 1;
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

    if (TRACE_SETUP) printf("[SIG] Set all signal handlers\n");
    return 0;
}

static void set_alarm_general(unsigned long us) {
    static const unsigned long mill = 1000000lu;
    const struct itimerval new_value = {
        .it_interval = {0, 0},
        .it_value = {us / mill, us % mill}
    };

    setitimer(ITIMER_REAL, &new_value, NULL);
    alarmed = 0;
}

void set_alarm() {
    set_alarm_general((unsigned long)timeout * 100000);
}

void unset_alarm() {
    set_alarm_general(0lu);
}

bool was_alarmed() {
    bool r = alarmed ? true : false;
    alarmed = 0;
    return r;
}

void test_alarm() {
    int s;
    errno = 0;

    // Short sleep test
    set_alarm_general(ALARM_TEST_SET);

    s = usleep(ALARM_TEST_SHORT_SLEEP);
    if (s != 0) {
        printf("[ALARM] Failed test_alarm() -- short sleep interrupted\n");
        exit(EXIT_FAILURE);
    }

    set_alarm_general(ALARM_TEST_SET);

    s = usleep(ALARM_TEST_LONG_SLEEP);
    if (s == 0 || errno != EINTR) {
        printf("[ALARM] Failed test_alarm() -- long sleep not interrupted\n");
        exit(EXIT_FAILURE);
    }

    unset_alarm();
    errno = 0;

    if (TRACE_SETUP) printf("[SETUP] Passed test_alarm()\n");
}
