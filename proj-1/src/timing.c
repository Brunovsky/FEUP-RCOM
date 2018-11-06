#include "timing.h"
#include "debug.h"
#include "options.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

static struct timespec timestamp[3];
static double times[3];

void print_stats(size_t i, size_t filesize) {
    static const char* stats_string = "[STATISTICS] %s\n"
        "==STATS==  Total Time %6.1lf ms                 \n"
        "==STATS==  Error probabilities:                 \n"
        "==STATS==    header-p      %1.6f                \n"
        "==STATS==    frame-p       %1.6f                \n"
        "==STATS==    FER I frames  %1.6f                \n"
        "==STATS==  Communication:                       \n"
        "==STATS==    Baudrate     %6d Baud (assume B/s) \n"
        "==STATS==    Packet Size  %6d bytes             \n"
        "==STATS==    Frame size   %6d bytes             \n"
        "==STATS==  Maximum theoretical efficiency:      \n"
        "==STATS==    %6.2f Bits/s                       \n"
        "==STATS==    %6.2f Bytes/s                      \n"
        "==STATS==    %6.2f Packets/s                    \n"
        "==STATS==  Observed efficiency:                 \n"
        "==STATS==    %6.2f Bits/s                       \n"
        "==STATS==    %6.2f Bytes/s                      \n"
        "==STATS==    %6.2f Packets/s                    \n";

    double ms = times[i];
    double h = h_error_prob, f = f_error_prob;

    double ferI = h + f - h * f;
    int frameIsize = 10 + packetsize;

    // Maximum
    double max_bits = baudrate * 8.0;
    double max_bytes = baudrate;
    double max_packs = (double)max_bytes / frameIsize;

    // Observed efficiency
    double obs_bits = 8.0 * 1000.0 * filesize / ms;
    double obs_bytes = 1000.0 * filesize / ms;
    double obs_packs = (double)obs_bytes / frameIsize;

    printf(stats_string, role_string, ms, h, f, ferI,
        baudrate, packetsize, frameIsize, max_bits, max_bytes,
        max_packs, obs_bits, obs_bytes, obs_packs);

    if (my_role == RECEIVER) {
        printf("==STATS== Note: Verify if correct packetsize is used\n");
    }
}

void begin_timing(size_t i) {    
    if (TRACE_TIME) {
        printf("[TIME] START Timing [%lu]\n", i);
    }

    times[i] = 0.0;
    clock_gettime(CLOCK_MONOTONIC, &timestamp[i]);
}

void end_timing(size_t i) {
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (TRACE_TIME) {
        printf("[TIME] END timing [%lu]\n", i);
    }

    double ts = (end.tv_sec - timestamp[i].tv_sec) * 1e3;
    double tns = (end.tv_nsec - timestamp[i].tv_nsec) / 1e6;
    
    times[i] = ts + tns;
    timestamp[i].tv_sec = 0; timestamp[i].tv_nsec = 0;

    printf("[STATS] Time [%lu] [ms=%.1lf]\n", i, times[i]);
}
