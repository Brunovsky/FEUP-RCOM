#include "ll-errors.h"
#include "debug.h"
#include "options.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static bool srand_seeded = false;

static void seed_srand() {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    srand(t.tv_nsec);
    srand_seeded = true;
}

static char corruptByte(char byte) {
    return byte ^ (1 << (rand() % 8));
}

static int introduceErrorsByte(string text) {
    int header_p = RAND_MAX * h_error_prob;
    int frame_p = RAND_MAX * f_error_prob;

    for (size_t i = 1; i < 4; ++i) {
        int header_r = rand();

        if (header_r < header_p) {
            char c = corruptByte(text.s[i]);

            if (TRACE_CORRUPTION) {
                printf("[CORR] [header i=%lu] Corrupted 0x%02x to 0x%02x\n",
                    i, (unsigned char)text.s[i], (unsigned char)c);
            }

            text.s[i] = c;
        }
    }

    for (size_t i = 4; i < text.len - 1; ++i) {
        int frame_r = rand();

        if (frame_r < frame_p) {
            char c = corruptByte(text.s[i]);

            if (TRACE_CORRUPTION) {
                printf("[CORR] [frame i=%lu] Corrupted 0x%02x to 0x%02x\n",
                    i, (unsigned char)text.s[i], (unsigned char)c);
            }

            text.s[i] = c;
        }
    }

    return 0;
}

static int introduceErrorsFrame(string text) {
    int header_p = RAND_MAX * h_error_prob;
    int frame_p = RAND_MAX * f_error_prob;

    int header_r = rand();
    size_t header_b = 1 + (rand() % 3);

    if (header_r < header_p) {
        char c = corruptByte(text.s[header_b]);

        if (TRACE_CORRUPTION) {
            printf("[CORR] [frame i=%lu] Corrupted 0x%02x to 0x%02x\n",
                header_b, (unsigned char)text.s[header_b], (unsigned char)c);
        }

        text.s[header_b] = c;
    }

    if (text.len > 5) {
        int frame_r = rand();
        size_t frame_b = 4 + (rand() % (text.len - 5));

        if (frame_r < frame_p) {
            char c = corruptByte(text.s[frame_b]);

            if (TRACE_CORRUPTION) {
                printf("[CORR] [frame i=%lu] Corrupted 0x%02x to 0x%02x\n",
                    frame_b, (unsigned char)text.s[frame_b], (unsigned char)c);
            }

            text.s[frame_b] = c;
        }
    }

    return 0;
}

int introduceErrors(string text) {
    if (!srand_seeded) seed_srand();

    if (error_type == ETYPE_BYTE) {
        return introduceErrorsByte(text);
    } else if (error_type == ETYPE_FRAME) {
        return introduceErrorsFrame(text);
    }

    return 0;
}
