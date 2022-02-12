#ifndef CLOCK_H
#define CLOCK_H

#include <time.h>

#define time_point struct timespec

void get_time_point(time_point* point);
double get_time_diff(const time_point* start, const time_point* end);
int is_time_set(const time_point* point);

struct timespec lftots(double seconds);

#endif
