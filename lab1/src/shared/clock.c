#include "clock.h"

void get_time_point(time_point* point) {
    timespec_get(point, TIME_UTC);
}

double get_time_diff(const time_point* start, const time_point* end) {
    return (end->tv_sec - start->tv_sec) +
            (end->tv_nsec - start->tv_nsec) / 1.e9;
}

int is_time_set(const time_point* point) {
    return point->tv_sec != 0 && point->tv_nsec != 0;
}

struct timespec lftots(double seconds) {
    struct timespec ts;
    ts.tv_sec = (long) seconds;
    ts.tv_nsec = (seconds - ts.tv_sec) * 1000000000;
    return ts;
}
