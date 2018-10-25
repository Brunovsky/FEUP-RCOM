#ifndef SIGNALS_H___
#define SIGNALS_H___

#include <stddef.h>
#include <stdbool.h>

int set_signal_handlers();

void set_alarm();

void unset_alarm();

bool was_alarmed();

void test_alarm();

int begin_timing(size_t i);

int end_timing(size_t i);

#endif // SIGNALS_H___
