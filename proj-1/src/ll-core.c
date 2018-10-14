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

// variable name
//   packet                |  Packet  |           Packet = File fragment
//    data            | PH |  Packet  |           PH = Packet Header
// text/frame    | FH | PH |  Packet  | FT |      FH/FT = Frame Header/Trailer

typedef enum {
    READ_PRE_FRAME, READ_START_FLAG, READ_A, READ_WITHIN_FRAME, READ_END_FLAG
} FrameReadState;

static int stuffData(string in, string* outp, char* bcc2) {
    size_t count = 0;

    for (size_t i = 0; i < in.len; ++i) {
        if (in.s[i] == FRAME_ESC || in.s[i] == FRAME_FLAG) {
            ++count;
        }
    }

    string stuffed_data;

    stuffed_data.len = in.len + count;
    stuffed_data.s = malloc((stuffed_data.len + 1) * sizeof(char));

    char parity = 0;

    for (size_t i = 0, j = 0; i < in.len; ++i, ++j) {
        switch (in.s[i]) {
        case FRAME_FLAG:
            stuffed_data.s[j++] = FRAME_ESC;
            stuffed_data.s[j] = FRAME_FLAG_STUFFING;
            break;
        case FRAME_ESC:
            stuffed_data.s[j++] = FRAME_ESC;
            stuffed_data.s[j] = FRAME_ESC_STUFFING;
            break;
        default:
            stuffed_data.s[j] = in.s[i];
        }

        parity ^= stuffed_data.s[j];
    }

    stuffed_data.s[stuffed_data.len] = '\0';

    *outp = stuffed_data;
    *bcc2 = parity;
    return 0;
}

static int destuffData(string in, string* outp, char* bcc2) {
    size_t count = 0;

    for (size_t i = 0; i < in.len; ++i) {
        if (in.s[i] == FRAME_ESC) {
            ++count;
        }
    }

    string destuffed_data;

    destuffed_data.len = in.len - count;
    destuffed_data.s = malloc((destuffed_data.len + 1) * sizeof(char));

    char parity = 0;

    for (size_t i = 0, j = 0; i < in.len; ++i, ++j) {
        if (in.s[i] == FRAME_ESC) {
            switch (in.s[++i]) {
            case FRAME_FLAG_STUFFING:
                destuffed_data.s[j] = FRAME_FLAG;
                break;
            case FRAME_ESC_STUFFING:
                destuffed_data.s[j] = FRAME_ESC;
                break;
            default:
                if (DEBUG) printf("[LL] Bad Escape [c=%c  %x]\n",
                    in.s[i], (unsigned char)in.s[i]);
                free(destuffed_data.s);
                return FRAME_READ_BAD_ESCAPE;
            }
        } else {
            destuffed_data.s[j] = in.s[i];
        }

        parity ^= destuffed_data.s[j];
    }

    destuffed_data.s[destuffed_data.len] = '\0';

    *outp = destuffed_data;
    *bcc2 = parity;
    return 0;
}

static int destuffText(string text, string* outp, char* bcc2) {
    string data;

    data.len = text.len - 6;
    data.s = malloc((data.len + 1) * sizeof(char));

    memcpy(data.s, text.s + 4, data.len);
    data.s[data.len] = '\0';

    int s = destuffData(data, outp, bcc2);

    free(data.s);
    return s;
}

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

        text.len = stuffed_data.len + 6;
        text.s = malloc((text.len + 1) * sizeof(char));

        text.s[0] = FRAME_FLAG;
        text.s[1] = f.a;
        text.s[2] = f.c;
        text.s[3] = f.a ^ f.c;
        memcpy(text.s + 4, stuffed_data.s, stuffed_data.len);
        text.s[text.len - 2] = bcc2;
        text.s[text.len - 1] = FRAME_FLAG;
        text.s[text.len] = '\0';

        free(stuffed_data.s);
        *textp = text;
        return 0;
    }
}

static int readText(int fd, string* textp) {
    string text;

    text.len = 0;
    text.s = malloc(8 * sizeof(char));

    size_t reserved = 8;

    FrameReadState state = READ_PRE_FRAME;

    while (state != READ_END_FLAG) {
        char readbuf[2];
        ssize_t s = read(fd, readbuf, 1);
        char c = readbuf[0];

        if (DEEP_DEBUG) {
            printf("[LLREAD] s:%01d  c:%02x  state:%01d  |  %c\n",
                (int)s, (unsigned char)c, state, c);
        }

        if (s == 0) continue;

        if (s == -1) {
            if (errno == EINTR) {
                if (DEBUG) printf("[LL] Read Error: Timeout [len=%lu]\n", text.len);
                free(text.s);
                return FRAME_READ_TIMEOUT;
            } else if (errno == EIO) {
                if (DEBUG) printf("[LL] Read Error: EIO [len=%lu]\n", text.len);
                continue;
            } else {
                if (DEBUG) printf("[LL] Read Error: Unknown [len=%lu]\n", text.len);
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
                    if (DEBUG) {
                        printf("[LL] Read: Bad A 0x%x, back to pre-frame\n", c);
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

    *textp = text;
    return 0;
}

int writeFrame(int fd, frame f) {
    string text;
    buildText(f, &text);

    write(fd, text.s, text.len);

    free(text.s);
    return 0;
}

int readFrame(int fd, frame* fp) {
    string text = {NULL, 0};

    set_alarm(timeout * 1000);
    int s = readText(fd, &text);
    unset_alarm();

    if (s != 0) return s;

    if (text.len < 5 || text.len == 6) {
        if (DEBUG) printf("[LL] Invalid Frame: BAD LENGTH\n");
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
        if (DEBUG) printf("[LL] Invalid Frame: BAD BCC1\n");
        free(text.s);
        return FRAME_READ_INVALID;
    }

    if (text.len > 6) {
        string data;
        char bcc2;
        int s = destuffText(text, &data, &bcc2);

        if (s != 0) return FRAME_READ_INVALID;

        char exp = text.s[text.len - 2];

        if (bcc2 != exp) {
            if (DEBUG) {
                printf("[LL] Invalid Frame: BAD BCC2 [calc=%x,exp=%x] [len=%lu]\n",
                    (unsigned char)bcc2, (unsigned char)exp, data.len);
                if (TEXT_DEBUG) print_stringn(data);
            }
            free(text.s);
            free(data.s);
            return FRAME_READ_INVALID;
        }

        f.data = data;
    }

    *fp = f;

    free(text.s);
    return FRAME_READ_OK;
}
