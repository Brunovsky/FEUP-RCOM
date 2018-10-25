#include "ll-core.h"
#include "options.h"
#include "signals.h"
#include "debug.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

/**
 * Performs stuffing on the string in, writing the result in the string out.
 * The bcc2 char is computed and appended to the out string, and also returned
 * through the out argument bcc2.
 *
 * This function does not fail.
 *
 * @param  in    String to be stuffed
 * @param  outp  [out] Stuffed string, appended with computed bcc2
 * @param  bcc2p [out] Computed bcc2
 * @return 0
 */
static int stuffData(string in, string* outp, char* bcc2p) {
    size_t stuff_count = 0;
    char parity = 0;

    for (size_t i = 0; i < in.len; ++i) {
        if (FRAME_MUST_ESCAPE(in.s[i])) ++stuff_count;
        parity ^= in.s[i];
    }

    bool stuff_bcc2 = FRAME_MUST_ESCAPE(parity);
    if (stuff_bcc2) ++stuff_count;

    string stuffed_data;
    stuffed_data.len = in.len + stuff_count + 1;
    stuffed_data.s = malloc((stuffed_data.len + 1) * sizeof(char));

    size_t j = 0;
    for (size_t i = 0; i < in.len; ++i) {
        switch (in.s[i]) {
        case FRAME_FLAG:
            stuffed_data.s[j++] = FRAME_ESC;
            stuffed_data.s[j++] = FRAME_FLAG_STUFFING;
            break;
        case FRAME_ESC:
            stuffed_data.s[j++] = FRAME_ESC;
            stuffed_data.s[j++] = FRAME_ESC_STUFFING;
            break;
        default:
            stuffed_data.s[j++] = in.s[i];
        }
    }

    switch (parity) {
    case FRAME_FLAG:
        stuffed_data.s[j++] = FRAME_ESC;
        stuffed_data.s[j++] = FRAME_FLAG_STUFFING;
        break;
    case FRAME_ESC:
        stuffed_data.s[j++] = FRAME_ESC;
        stuffed_data.s[j++] = FRAME_ESC_STUFFING;
        break;
    default:
        stuffed_data.s[j++] = parity;
    }

    stuffed_data.s[stuffed_data.len] = '\0';
    assert(j == stuffed_data.len);

    *outp = stuffed_data;
    *bcc2p = parity;
    return 0;
}

/**
 * Performs destuffing on the string in, writing the result in the string out.
 * A bcc2 is presumed to be found at the end of in, possibly escaped.
 * This character is removed, and not written to string out.
 * The data's bcc2.is computed returned through the out argument bcc2.
 *
 * @param  in    String to be destuffed
 * @param  outp  [out] Destuffed string, without appended bcc2
 * @param  bcc2p [out] Computed bcc2
 * @return 0 if successful
 *         FRAME_READ_BAD_ESCAPE if there is a badly escaped character
 *         FRAME_READ_BAD_BCC2 if bcc2 check does not pass
 */
static int destuffData(string in, string* outp, char* bcc2p) {
    size_t count = 0;

    for (size_t i = 0; i < in.len; ++i) {
        if (in.s[i] == FRAME_ESC) ++count;
    }

    string destuffed_data;
    destuffed_data.len = in.len - count - 1;
    destuffed_data.s = malloc((destuffed_data.len + 1) * sizeof(char));

    char parity = 0;

    size_t j = 0;
    for (size_t i = 0; i < in.len; ++i, ++j) {
        if (in.s[i] == FRAME_ESC) {
            switch (in.s[++i]) {
            case FRAME_FLAG_STUFFING:
                destuffed_data.s[j] = FRAME_FLAG;
                break;
            case FRAME_ESC_STUFFING:
                destuffed_data.s[j] = FRAME_ESC;
                break;
            default:
                if (TRACE_LL) {
                    printf("[LL] Bad Escape [c=%c  0x%02x]\n",
                        in.s[i], (unsigned char)in.s[i]);
                }
                free(destuffed_data.s);
                return FRAME_READ_BAD_ESCAPE;
            }
        } else {
            destuffed_data.s[j] = in.s[i];
        }

        parity ^= destuffed_data.s[j];
    }

    assert(j == destuffed_data.len + 1);

    char bcc2 = destuffed_data.s[destuffed_data.len];
    parity ^= bcc2;

    if (bcc2 != parity) {
        if (TRACE_LL) {
            printf("[LL] Invalid Frame: BAD BCC2 [calc=0x%02x,read=0x%02x] [len=%lu]\n",
                (unsigned char)parity, (unsigned char)bcc2, destuffed_data.len);
        }
        free(destuffed_data.s);
        return FRAME_READ_BAD_BCC2;
    }

    destuffed_data.s[destuffed_data.len] = '\0'; // clear bcc2

    *outp = destuffed_data;
    *bcc2p = parity;
    return 0;
}

/**
 * A wrapper function for destuffData, which first extracts
 * the data fragment a string from the frame string text.
 *
 * @param  text Full frame text
 * @param  outp [out] Destuffed data string
 * @param  bcc2 [out] Computed bcc2
 * @return 0 if successful
 */
static int destuffText(string text, string* outp, char* bcc2) {
    string data;

    data.len = text.len - 5;
    data.s = malloc((data.len + 1) * sizeof(char));

    memcpy(data.s, text.s + 4, data.len);
    data.s[data.len] = '\0';

    int s = destuffData(data, outp, bcc2);

    free(data.s);
    return s;
}

/**
 * Constructs the frame string text from a frame struct.
 *
 * This function does not fail.
 *
 * @param  f     Frame to be stringified
 * @param  textp [out] Stringified frame
 * @return 0
 */
static int buildText(frame f, string* textp) {
    if (f.data.s == NULL) {
        // S or U frame (control frame)
        string text;

        text.len = 5;
        text.s = malloc(6 * sizeof(char));

        text.s[0] = FRAME_FLAG;
        text.s[1] = f.a;
        text.s[2] = f.c;
        text.s[3] = f.a ^ f.c;
        text.s[4] = FRAME_FLAG;
        text.s[5] = '\0';

        *textp = text;
        return 0;
    } else {
        // I frame (data frame)
        string stuffed_data;
        char bcc2;
        stuffData(f.data, &stuffed_data, &bcc2);

        string text;

        text.len = stuffed_data.len + 5;
        text.s = malloc((text.len + 1) * sizeof(char));

        text.s[0] = FRAME_FLAG;
        text.s[1] = f.a;
        text.s[2] = f.c;
        text.s[3] = f.a ^ f.c;
        memcpy(text.s + 4, stuffed_data.s, stuffed_data.len);
        text.s[text.len - 1] = FRAME_FLAG;
        text.s[text.len] = '\0';

        free(stuffed_data.s);
        *textp = text;
        return 0;
    }
}

/**
 * State machine for readText function
 */
typedef enum {
    READ_PRE_FRAME, READ_START_FLAG, READ_WITHIN_FRAME, READ_END_FLAG
} FrameReadState;

static char corruptByte(char byte) {
    return byte ^ (1 << (rand() % 8));
}

static int introduceErrors(string text) {
    if (error_type == ETYPE_BYTE) {
        return introduceErrorsByte(text);
    } else if (error_type == ETYPE_FRAME) {
        return introduceErrorsFrame(text);
    }
}

static int introduceErrorsByte(string text) {
    int header_p = RAND_MAX * h_error_prob;
    int frame_p = RAND_MAX * f_error_prob;

    for (size_t i = 1; i < 4; ++i) {
        int header_r = rand();

        if (header_r < header_p) {
            char c = corruptByte(text.s[i]);

            if (TRACE_ERRORS) {
                printf("[ERR] Corrupted %02x to %02x [header i=%d]",
                    text.s[i], c, i);
            }

            text.s[i] = c;
        }
    }

    for (size_t i = 4; i < text.len - 1; ++i) {
        int frame_r = rand();

        if (frame_r < frame_p) {
            char c = corruptByte(text.s[i]);

            if (TRACE_ERRORS) {
                printf("[ERR] Corrupted %02x to %02x [frame i=%d]",
                    text.s[i], c, i);
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
    int header_b = 1 + (rand() % 3);

    if (header_r < header_p) {
        char c = corruptByte(text.s[header_b]);

        if (TRACE_ERRORS) {
            printf("[ERR] Corrupted %02x to %02x [frame i=%d]",
                text.s[header_b], c, header_b);
        }

        text.s[header_b] = c;
    }

    if (text.len > 5) {
        int frame_r = rand();
        int frame_b = 4 + (rand() % (text.len - 5));

        if (frame_r < frame_p) {
            char c = corruptByte(text.s[frame_b]);

            if (TRACE_ERRORS) {
                printf("[ERR] Corrupted %02x to %02x [frame i=%d]",
                    text.s[frame_b], c, frame_b);
            }

            text.s[frame_b] = c;
        }
    }

    return 0;
}

/**
 * Primary Link Layer reading function.
 *
 * State is controlled by the FrameReadState enum. It starts reading
 * the frame, including one initial and one terminal flag characters,
 * once it encounters a non-flag character after a flag. This non-flag
 * character must be a valid A character, as tested by FRAME_VALID_A,
 * otherwise it assumes it is reading noise.
 *
 * Enable DEEP_DEBUG in debug.h to echo characters read in the terminal.
 *
 * @param  fd    Communications file descriptor
 * @param  textp [out] Frame text read
 * @return 0 if successful
 *         FRAME_READ_TIMEOUT if a timeout occurred
 *         FRAME_READ_INVALID if some other unknown error occurred
 */
static int readText(int fd, string* textp) {
    string text;

    text.len = 0;
    text.s = malloc(8 * sizeof(char));

    size_t reserved = 8;

    FrameReadState state = READ_PRE_FRAME;

    int timed = 0;

    while (state != READ_END_FLAG) {
        char readbuf[2];
        ssize_t s = read(fd, readbuf, 1);
        char c = readbuf[0];

        if (DEEP_DEBUG) {
            printf("[LLREAD] s:%01d  c:%02x  state:%01d  |  %c\n",
                (int)s, (unsigned char)c, state, c);
        }

        if (s == 0) {
            if (++timed == timeout) {
                if (TRACE_LL) {
                    printf("[LL] Reading Timeout [len=%lu]\n", text.len);
                }
                free(text.s);
                return FRAME_READ_TIMEOUT;
            } else {
                continue;
            }
        }

        if (s == -1) {
            if (errno == EINTR) {
                if (TRACE_LL) {
                    printf("[LL] Read Error: EINTR [len=%lu]\n", text.len);
                }
                continue;
            } else if (errno == EIO) {
                if (TRACE_LL) {
                    printf("[LL] Read Error: EIO [len=%lu]\n", text.len);
                }
                continue;
            } else if (errno == EAGAIN) {
                if (TRACE_LL) {
                    printf("[LL] Read Error: EAGAIN [len=%lu]\n", text.len);
                }
                continue;
            } else {
                if (TRACE_LL) {
                    printf("[LL] Read Error: %s [len=%lu]\n", strerror(errno), text.len);
                }
                free(text.s);
                return FRAME_READ_INVALID;
            }
        }

        if (text.len == reserved) {
            text.s = realloc(text.s, 2 * reserved * sizeof(char));
            reserved *= 2;
        }

        switch (state) {
        case READ_PRE_FRAME:
            if (c == FRAME_FLAG) {
                state = READ_START_FLAG;
                text.s[text.len++] = FRAME_FLAG;
            }
            break;
        case READ_START_FLAG:
            if (c != FRAME_FLAG) {
                if (FRAME_VALID_A(c)) {
                    state = READ_WITHIN_FRAME;
                    text.s[text.len++] = c;
                } else {
                    if (TRACE_LL) {
                        printf("[LL] Read: Bad A 0x%02x, back to pre-frame\n", c);
                    }
                    state = READ_PRE_FRAME;
                    text.len = 0;
                }
            }
            break;
        case READ_WITHIN_FRAME:
            if (c == FRAME_FLAG) {
                state = READ_END_FLAG;
                text.s[text.len++] = FRAME_FLAG;
            } else {
                text.s[text.len++] = c;
            }
            break;
        case READ_END_FLAG:
        default:
            break;
        }
    }

    text.s[text.len] = '\0';

    introduceErrors(text);

    *textp = text;
    return 0;
}

/**
 * Writes a frame to communication device.
 * @param  fd Communications file descriptor
 * @param  f  Frame to be written
 * @return 0
 */
int writeFrame(int fd, frame f) {
    string text;
    buildText(f, &text);

    set_alarm();
    errno = 0;
    ssize_t s = write(fd, text.s, text.len);

    int err = errno;
    bool b = was_alarmed();
    unset_alarm();

    if (b || err == EINTR) {
        if (TRACE_LL) {
            printf("[LL] Write Frame: Timeout [alarm=%d s=%d errno=%d] [%s]\n",
                (int)b, (int)s, err, strerror(err));
        }
        free(text.s);
        return FRAME_WRITE_TIMEOUT;
    } else {
        free(text.s);
        return FRAME_WRITE_OK;
    }
}

/**
 * Reads a frame from communication device.
 *
 * @param  fd Communications file descriptor
 * @param  fp [out] Frame read
 * @return FRAME_READ_OK if successful
 *         FRAME_READ_TIMEOUT if a timeout occurred while reading
 *         FRAME_READ_INVALID if the frame read is invalid
 */
int readFrame(int fd, frame* fp) {
    string text;

    int s = readText(fd, &text);

    if (s != 0) return s;

    if (text.len < 5 || text.len == 6) {
        if (TRACE_LL) printf("[LL] Read Frame: BAD LENGTH\n");
        free(text.s);
        return FRAME_READ_INVALID;
    }

    frame f = {
        .a = text.s[1],
        .c = text.s[2],
        .data = {NULL, 0}
    };

    char bcc1 = text.s[3];

    if (bcc1 != (f.a ^ f.c)) {
        if (TRACE_LL) printf("[LL] Read Frame: BAD BCC1\n");
        free(text.s);
        return FRAME_READ_INVALID;
    }

    if (text.len > 6) {
        string data;
        char bcc2;
        int s = destuffText(text, &data, &bcc2);

        if (s != 0) {
            free(text.s);
            return FRAME_READ_INVALID;
        }

        f.data = data;
    }

    *fp = f;

    free(text.s);
    return FRAME_READ_OK;
}
