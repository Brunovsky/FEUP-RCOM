#include "ll-interface.h"
#include "ll-frames.h"
#include "options.h"
#include "debug.h"

#include <stdlib.h>
#include <stdio.h>

/**
 * llopen for T
 * 
 * @param  fd Link layer's file descriptor
 * @return LL_OK if llopen succeeded,
 *         LL_NO_TIME_RETRIES if timeouts maxed out,
 *         LL_NO_ANSWER_RETRIES if answer errors maxed out.
 */
static int llopen_transmitter(int fd) {
    int time_count = 0, answer_count = 0;

    while (time_count < time_retries && answer_count < answer_retries) {
        int s = writeSETframe(fd);
        if (s != FRAME_WRITE_OK) {
            ++time_count;
            continue;
        }

        frame f;
        s = readFrame(fd, &f);

        switch (s) {
        case FRAME_READ_OK:
            if (isUAframe(f)) {
                if (TRACE_LL || TRACE_FILE) {
                    printf("[LL] llopen (T) OK\n");
                }
                return LL_OK;
            }
            // FALLTHROUGH
        case FRAME_READ_INVALID:
            if (LL_ASSUME_OK) {
                if (TRACE_LL || TRACE_FILE) {
                    printf("[LL] llopen (T) ASSUME UA OK\n");
                }
                return LL_OK;
            }
            ++answer_count;
            break;
        case FRAME_READ_TIMEOUT:
            ++time_count;
            break;
        }
    }

    if (time_count == time_retries) {
        printf("[LL] llopen (T) FAILED: %d time retries ran out\n", time_retries);
        return LL_NO_TIME_RETRIES;
    } else {
        printf("[LL] llopen (T) FAILED: %d answer retries ran out\n", answer_retries);
        return LL_NO_ANSWER_RETRIES;
    }
}

/**
 * llopen for R
 * 
 * @param  fd Link layer's file descriptor
 * @return LL_OK if llopen succeeded,
 *         LL_NO_TIME_RETRIES if timeouts maxed out,
 *         LL_NO_ANSWER_RETRIES if answer errors maxed out.
 */
static int llopen_receiver(int fd) {
    int time_count = 0, answer_count = 0;

    while (time_count < time_retries && answer_count < answer_retries) {
        frame f;
        int s = readFrame(fd, &f);

        switch (s) {
        case FRAME_READ_OK:
            if (isSETframe(f)) {
                writeUAframe(fd);
                if (TRACE_LL || TRACE_FILE) {
                    printf("[LL] llopen (R) OK\n");
                }
                return LL_OK;
            }
            // FALLTHROUGH
        case FRAME_READ_INVALID:
            ++answer_count;
            break;
        case FRAME_READ_TIMEOUT:
            ++time_count;
            break;
        }
    }

    if (time_count == time_retries) {
        printf("[LL] llopen (R) FAILED: %d time retries ran out\n", time_retries);
        return LL_NO_TIME_RETRIES;
    } else {
        printf("[LL] llopen (R) FAILED: %d answer retries ran out\n", answer_retries);
        return LL_NO_ANSWER_RETRIES;
    }
}

/**
 * llclose for T
 * 
 * @param  fd Link layer's file descriptor
 * @return LL_OK if llclose succeeded,
 *         LL_NO_TIME_RETRIES if timeouts maxed out,
 *         LL_NO_ANSWER_RETRIES if answer errors maxed out.
 */
static int llclose_transmitter(int fd) {
    int time_count = 0, answer_count = 0;

    while (time_count < time_retries && answer_count < answer_retries) {
        int s = writeDISCframe(fd);
        if (s != FRAME_WRITE_OK) {
            ++time_count;
            continue;
        }

        frame f;
        s = readFrame(fd, &f);

        switch (s) {
        case FRAME_READ_OK:
            if (isDISCframe(f)) {
                writeUAframe(fd);
                if (TRACE_LL || TRACE_FILE) {
                    printf("[LL] llclose (T) OK\n");
                }
                return LL_OK;
            }
            // FALLTHROUGH
        case FRAME_READ_INVALID:
            ++answer_count;
            break;
        case FRAME_READ_TIMEOUT:
            ++time_count;
            break;
        }
    }

    if (time_count == time_retries) {
        printf("[LL] llclose (T) FAILED: %d time retries ran out\n", time_retries);
        return LL_NO_TIME_RETRIES;
    } else {
        printf("[LL] llclose (T) FAILED: %d answer retries ran out\n", answer_retries);
        return LL_NO_ANSWER_RETRIES;
    }
}

/**
 * State machine for llclose_receiver function
 */
typedef enum {
    WAIT_DISC, GOOD_DISC, SEND_DISC, WAIT_ANSWER, TEST_TRANSMITTER
} LLCloseState;

// So here we have an issue. The protocol goes:
//    (1) Wait for a DISC.
//        If this is difficult keep trying/waiting to read a DISC,
//        and increment the counts accordingly.
//    (2) Finally we read a good DISC.
//    (3) Send DISC.
//    (4) Wait for an answer.
//    (5.1) Bad answer: ???
//    (5.2) Good answer:
//        If the answer is UA all is good and we leave.
//        If the answer is DISC we got back to (2).
// 
// Consider a situation where the line has a lot of noise.
// After we answer a good DISC with a DISC,
// the answer we receive at (4) is all mangled up.
// We can't assume UA in (5.1) similarly to how we did in llopen
// because if the answer is DISC we'd let T hung up,
// which isn't nice enough. So we test T. Set a testing flag to change
// the usual behaviour (test_status) and do as follows:
// 
//    (5.1) Bad answer: Respond with DISC.
//    (5.1.1) If the write times out, the answer was a UA and T is gone.
//    (5.1.2) If the read times out, no answer, same thing.
//    (5.1.3) If neither times out, there is an answer, so we assume
//        we had gotten a DISC in (4) and goto (2).

/**
 * llclose for R
 * 
 * @param  fd Link layer's file descriptor
 * @return LL_OK if llclose succeeded,
 *         LL_NO_TIME_RETRIES if timeouts maxed out,
 *         LL_NO_ANSWER_RETRIES if answer errors maxed out.
 */
static int llclose_receiver(int fd) {
    int time_count = 0, answer_count = 0;

    int test_status = false;

    while (time_count < time_retries && answer_count < answer_retries) {
        frame f;
        int s = readFrame(fd, &f); // 1

        switch (s) {
        case FRAME_READ_OK:
            if (isDISCframe(f)) { // 2
answer:
                s = writeDISCframe(fd); // 3
                if (s != FRAME_WRITE_OK) {
                    if (test_status) {
                        if (TRACE_LL || TRACE_FILE) {
                            printf("[LL] llclose (R) WRITE TIMEOUT ASSUME OK\n");
                        }
                        return LL_OK;
                    } else {
                        ++time_count;
                        continue;
                    }
                }

                s = readFrame(fd, &f); // 4

                switch (s) {
                case FRAME_READ_OK: // 5.2
                    if (isUAframe(f)) {
                        if (TRACE_LL || TRACE_FILE) {
                            printf("[LL] llclose (R) OK\n");
                        }
                        return LL_OK;
                    }
                    if (isDISCframe(f)) {
                        goto answer;
                    }
                    // FALLTHROUGH
                case FRAME_READ_INVALID: // 5.1
                    if (LL_ASSUME_OK) {
                        test_status = !test_status;
                        goto answer;
                    }
                    ++answer_count;
                    break;
                case FRAME_READ_TIMEOUT:
                    if (test_status) {
                        if (TRACE_LL || TRACE_FILE) {
                            printf("[LL] llclose (R) READ TIMEOUT ASSUME OK\n");
                        }
                        return LL_OK;
                    }
                    test_status = false;
                    ++time_count;
                    break;
                }

                break;
            }
            // FALLTHROUGH
        case FRAME_READ_INVALID:
            ++answer_count;
            break;
        case FRAME_READ_TIMEOUT:
            ++time_count;
            break;
        }
    }

    if (time_count == time_retries) {
        printf("[LL] llclose (R) FAILED: %d time retries ran out\n", time_retries);
        return LL_NO_TIME_RETRIES;
    } else {
        printf("[LL] llclose (R) FAILED: %d answer retries ran out\n", answer_retries);
        return LL_NO_ANSWER_RETRIES;
    }
}

/**
 * @param  fd Link layer's file descriptor
 * @return LL_OK if llopen succeeded,
 *         LL_NO_TIME_RETRIES if timeouts maxed out,
 *         LL_NO_ANSWER_RETRIES if answer errors maxed out.
 */
int llopen(int fd) {
    if (my_role == TRANSMITTER) {
        return llopen_transmitter(fd);
    } else {
        return llopen_receiver(fd);
    }
}

/**
 * @param fd      Link layer's file descriptor
 * @param message String to be sent over LL
 * @return LL_OK if llwrite succeeded,
 *         LL_NO_TIME_RETRIES if timeouts maxed out,
 *         LL_NO_ANSWER_RETRIES if answer errors maxed out.
 */
int llwrite(int fd, string message) {
    static int index = 0; // only supports one fd.

    int time_count = 0, answer_count = 0;

    while (time_count < time_retries && answer_count < answer_retries) {
        int s = writeIframe(fd, message, index);
        if (s != FRAME_WRITE_OK) {
            ++time_count;
            continue;
        }

        frame f;
        s = readFrame(fd, &f);

        switch (s) {
        case FRAME_READ_OK:
            if (isRRframe(f, index + 1) || isREJframe(f, index + 1)) {
                ++index;
                if (TRACE_LL) {
                    printf("[LL] llwrite OK [index=%d]\n", index);
                }
                return LL_OK;
            } else if (isRRframe(f, index) || isREJframe(f, index)) {
                ++answer_count;
            } else {
                if (TRACE_LL) {
                    printf("[LL] llwrite: invalid response (not RR or REJ)\n");
                }
                ++answer_count;
            }
            break;
        case FRAME_READ_INVALID:
            ++answer_count;
            break;
        case FRAME_READ_TIMEOUT:
            ++time_count;
            break;
        }
    }

    if (time_count == time_retries) {
        printf("[LL] llwrite FAILED: %d time retries ran out\n", time_retries);
        return LL_NO_TIME_RETRIES;
    } else {
        printf("[LL] llwrite FAILED: %d answer retries ran out\n", answer_retries);
        return LL_NO_ANSWER_RETRIES;
    }
}

/**
 * @param fd       Link layer's file descriptor
 * @param messagep Where to store the read message
 * @return LL_OK if llread succeeded,
 *         LL_NO_TIME_RETRIES if timeouts maxed out,
 *         LL_NO_ANSWER_RETRIES if answer errors maxed out.
 */
int llread(int fd, string* messagep) {
    static int index = 0; // only supports one fd.

    int time_count = 0, answer_count = 0;

    while (time_count < time_retries && answer_count < answer_retries) {
        frame f;
        int s = readFrame(fd, &f);

        switch (s) {
        case FRAME_READ_OK:
            if (isIframe(f, index)) {
                writeRRframe(fd, ++index);
                *messagep = f.data;
                if (TRACE_LL) {
                    printf("[LL] llread OK [index=%d]\n", index);
                }
                return LL_OK;
            } else if (isIframe(f, index + 1)) {
                writeRRframe(fd, index);
                ++answer_count;
                if (TRACE_LL) {
                    printf("[LL] llread: Expected frame %d, got frame %d\n",
                        index % 2, (index + 1) % 2);
                }
            }
            break;
        case FRAME_READ_INVALID:
            writeREJframe(fd, index);
            ++answer_count;
            break;
        case FRAME_READ_TIMEOUT:
            ++time_count;
            break;
        }
    }

    if (time_count == time_retries) {
        printf("[LL] llwrite FAILED: %d time retries ran out\n", time_retries);
        return LL_NO_TIME_RETRIES;
    } else {
        printf("[LL] llwrite FAILED: %d answer retries ran out\n", answer_retries);
        return LL_NO_ANSWER_RETRIES;
    }
}

/**
 * @param  fd Link layer's file descriptor
 * @return LL_OK if llclose succeeded,
 *         LL_NO_TIME_RETRIES if timeouts maxed out,
 *         LL_NO_ANSWER_RETRIES if answer errors maxed out.
 */
int llclose(int fd) {
    if (my_role == TRANSMITTER) {
        return llclose_transmitter(fd);
    } else {
        return llclose_receiver(fd);
    }
}
