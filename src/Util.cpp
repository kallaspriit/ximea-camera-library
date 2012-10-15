#include "Util.h"

#include <sys/time.h>

double Util::millitime() {
    timeval timeOfDay;

    gettimeofday(&timeOfDay, 0);

    long seconds  = timeOfDay.tv_sec;
    long useconds = timeOfDay.tv_usec;

    return (double)seconds + (double)useconds / 1000000.0d;
}
