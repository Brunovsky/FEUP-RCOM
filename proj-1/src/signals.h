#ifndef SIGNALS_H___
#define SIGNALS_H___

int set_signal_handlers();

void set_alarm(int microseconds);

void unset_alarm();

void test_alarm();

#endif // SIGNALS_H___
