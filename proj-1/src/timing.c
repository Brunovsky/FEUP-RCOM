#include "timing.h"
#include "ll-errors.h"
#include "debug.h"
#include "options.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

static struct timespec timestamp[3];
static double times[3];

size_t number_of_packets(size_t filesize) {
    return (filesize + packetsize - 1) / packetsize;
}

double average_packetsize(size_t filesize) {
    return (double)filesize / number_of_packets(filesize);
}

static void print_stats_receiver(size_t i, size_t filesize) {
    static const char* stats_string = "[STATISTICS RECEIVER]\n"
        "==STATS==  Total Time:  %.5lf seconds                \n"
        "==STATS==  Error probabilities:                      \n"
        "==STATS==    header-p      %7.5f                     \n"
        "==STATS==    frame-p       %7.5f                     \n"
        "==STATS==    FER I frames  %7.5f                     \n"
        "==STATS==  Communication:                            \n"
        "==STATS==    Baudrate     %7d Symbols/s              \n"
        "==STATS==    Packet Size  %7d bytes (average)        \n"
        "==STATS==    I frame Size %7d bytes (average)        \n"
        "==STATS==    %6d Data packets (+2 total)             \n"
        "==STATS==  Efficiency:                               \n"
        "==STATS==   Observed | Maximum(b) | Maximum(B)       \n"
        "==STATS==    %11.2f | %11.2f | %11.2f Bits/s         \n"
        "==STATS==    %11.2f | %11.2f | %11.2f Bytes/s        \n"
        "==STATS==    %11.2f | %11.2f | %11.2f Packs/s        \n"
        "==STATS==    %6d Timeouts                            \n"
        "==STATS==  Frames Received:                          \n"
        "==STATS==    %8d I frames                            \n"
        "==STATS==    %8d Invalid or unexpected               \n"
        "==STATS==  Frames Transmitted:                       \n"
        "==STATS==    %6d RR frames                           \n"
        "==STATS==      %6d RR0  %6d RR1                      \n"
        "==STATS==    %6d REJ frames                          \n"
        "==STATS==      %6d REJ0  %6d REJ1                    \n"
        "==STATS==  Reading Errors:                           \n"
        "==STATS==    %6d Bad frame length                    \n"
        "==STATS==    %6d Bad BCC1                            \n"
        "==STATS==    %6d Bad BCC2                            \n"
        "==STATS==\n";

    double ms = times[i];
    double s = ms / 1000.0;
    double h = h_error_prob, f = f_error_prob;

    double ferI = h + f - (h * f);
    int numpackets = number_of_packets(filesize);
    int average = average_packetsize(filesize);
    int frameIsize = 10 + average;

    // Observed
    double obs_bits = 8.0 * filesize / s;
    double obs_bytes = filesize / s;
    double obs_packs = obs_bytes / average_packetsize(filesize);

    // Maximum
    double max_bits = baudrate;
    double max_bytes = baudrate / 8.0;
    double max_packs = max_bytes / average_packetsize(filesize);

    printf(stats_string,
        s, h, f, ferI,
        baudrate,
        average,
        frameIsize,
        numpackets,
        obs_bits, max_bits, max_bits * 8.0,
        obs_bytes, max_bytes, max_bytes * 8.0,
        obs_packs, max_packs, max_packs * 8.0,
        counter.timeout,
        counter.in.I,
        counter.invalid,
        counter.out.RR[0] + counter.out.RR[1],
        counter.out.RR[0], counter.out.RR[1],
        counter.out.REJ[0] + counter.out.REJ[1],
        counter.out.REJ[0], counter.out.REJ[1],
        counter.read.len,
        counter.read.bcc1,
        counter.read.bcc2);
}

static void print_stats_transmitter(size_t i, size_t filesize) {
    static const char* stats_string = "[STATISTICS TRANSMITTER]\n"
        "==STATS==  Total Time:  %.5lf seconds                \n"
        "==STATS==  Error probabilities:                      \n"
        "==STATS==    header-p      %7.5f                     \n"
        "==STATS==  Communication:                            \n"
        "==STATS==    Baudrate     %7d Symbols/s              \n"
        "==STATS==    Packet Size  %7d bytes (average)        \n"
        "==STATS==    I frame Size %7d bytes (average)        \n"
        "==STATS==    %6d Data packets (+2 total)             \n"
        "==STATS==  Efficiency:                               \n"
        "==STATS==   Observed | Maximum(b) | Maximum(B)       \n"
        "==STATS==    %11.2f | %11.2f | %11.2f Bits/s         \n"
        "==STATS==    %11.2f | %11.2f | %11.2f Bytes/s        \n"
        "==STATS==    %11.2f | %11.2f | %11.2f Packs/s        \n"
        "==STATS==    %6d Timeouts                            \n"
        "==STATS==  Frames Transmitted:                       \n"
        "==STATS==    %8d I frames                            \n"
        "==STATS==  Frames Received:                          \n"
        "==STATS==    %6d RR frames                           \n"
        "==STATS==      %6d RR0  %6d RR1                      \n"
        "==STATS==    %6d REJ frames                          \n"
        "==STATS==      %6d REJ0  %6d REJ1                    \n"
        "==STATS==  %8d Invalid or unexpected                 \n"
        "==STATS==  Reading Errors:                           \n"
        "==STATS==    %6d Bad frame length                    \n"
        "==STATS==    %6d Bad BCC1                            \n"
        "==STATS==    %6d Bad BCC2                            \n"
        "==STATS==\n";

    double ms = times[i];
    double s = ms / 1000.0;
    double h = h_error_prob;

    int numpackets = number_of_packets(filesize);
    int average = average_packetsize(filesize);
    int frameIsize = 10 + average;

    // Observed
    double obs_bits = 8.0 * filesize / s;
    double obs_bytes = filesize / s;
    double obs_packs = obs_bytes / average_packetsize(filesize);

    // Maximum
    double max_bits = baudrate;
    double max_bytes = baudrate / 8.0;
    double max_packs = max_bytes / average_packetsize(filesize);

    printf(stats_string,
        s, h,
        baudrate,
        average,
        frameIsize,
        numpackets,
        obs_bits, max_bits, max_bits * 8.0,
        obs_bytes, max_bytes, max_bytes * 8.0,
        obs_packs, max_packs, max_packs * 8.0,
        counter.timeout,
        counter.out.I,
        counter.in.RR[0] + counter.in.RR[1],
        counter.in.RR[0], counter.in.RR[1],
        counter.in.REJ[0] + counter.in.REJ[1],
        counter.in.REJ[0], counter.in.REJ[1],
        counter.invalid,
        counter.read.len,
        counter.read.bcc1,
        counter.read.bcc2);
}

void print_stats(size_t i, size_t filesize) {
    if (my_role == RECEIVER) {
        print_stats_receiver(i, filesize);
    } else {
        print_stats_transmitter(i, filesize);
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
