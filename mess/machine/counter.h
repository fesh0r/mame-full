#ifndef COUNTER_H
#define COUNTER_H

/* counter.h - Generic implementation of timed counter register */

void *counter_set(unsigned long value, double period, int param, void(*callback)(int));
unsigned long counter_get_value(void *which);
void counter_set_value(void *which, unsigned long value);
void counter_set_period(void *which, double period);
void counter_remove(void *which);

#endif /* COUNTER_H */
