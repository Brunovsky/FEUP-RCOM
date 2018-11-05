#ifndef SIGNALS_H___
#define SIGNALS_H___

#include <stdbool.h>

int set_signal_handlers();

void set_alarm();

void unset_alarm();

bool was_alarmed();

void test_alarm();

#endif // SIGNALS_H___
