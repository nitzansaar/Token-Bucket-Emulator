#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "warmup2.h"

void TimeNow(struct timeval *t)
{
    if (gettimeofday(t, NULL) != 0) {
        perror("gettimeofday");
        exit(1);
    }
}

void TimeElapsed(const struct timeval *t0, const struct timeval *t,
                 struct timeval *diff)
{
    timersub(t, t0, diff);
}

void TimePrintEvent(const struct timeval *t0, const struct timeval *t,
                    const char *msg)
{
    struct timeval diff;
    long total_usec;
    long ms_whole;
    long ms_frac;

    TimeElapsed(t0, t, &diff);
    total_usec = (long)diff.tv_sec * 1000000L + (long)diff.tv_usec;
    if (total_usec < 0) {
        total_usec = 0;
    }
    ms_whole = total_usec / 1000L;
    ms_frac = total_usec % 1000L;
    printf("%08ld.%03ldms: %s\n", ms_whole, ms_frac, msg);
}
