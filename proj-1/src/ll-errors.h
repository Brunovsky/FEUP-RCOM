#ifndef LL_ERRORS_H___
#define LL_ERRORS_H___

#include "strings.h"

typedef struct {
    size_t len, bcc1, bcc2;
} read_count_t;

typedef struct {
    size_t I, RR[2], REJ[2], SET, DISC, UA;
} frame_count_t;

typedef struct {
    frame_count_t in;
    frame_count_t out;
    read_count_t read;
    size_t invalid;
    size_t timeout;
    size_t bcc_errors;
} communication_count_t;

extern communication_count_t counter;

int introduceErrors(string text);

#endif // LL_ERRORS_H___
