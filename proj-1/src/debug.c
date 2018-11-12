#include "debug.h"

communication_count_t counter;

void reset_counter() {
    communication_count_t dummy = {0};
    counter = dummy;
}
