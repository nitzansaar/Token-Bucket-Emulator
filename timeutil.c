#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>

#include "warmup2.h"

#define MAX_INTERVAL_MS 10000

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

void TimeAddMs(const struct timeval *base, int ms, struct timeval *out)
{
    struct timeval delta;

    delta.tv_sec = ms / 1000;
    delta.tv_usec = (ms % 1000) * 1000;
    timeradd(base, &delta, out);
}

void TimeSleepRemaining(const struct timeval *expected)
{
    struct timeval now;
    struct timeval rem;

    TimeNow(&now);
    timersub(expected, &now, &rem);
    if (rem.tv_sec < 0 || (rem.tv_sec == 0 && rem.tv_usec <= 0)) {
        return;
    }
    /* select() can sleep multi-second intervals; usleep() cannot on some systems */
    (void)select(0, NULL, NULL, NULL, &rem);
}

void TimeFormatInterval(const struct timeval *diff, char *buf, size_t n)
{
    long total_usec;

    total_usec = (long)diff->tv_sec * 1000000L + (long)diff->tv_usec;
    if (total_usec < 0) {
        total_usec = 0;
    }
    snprintf(buf, n, "%ld.%03ldms", total_usec / 1000L, total_usec % 1000L);
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

int TimeInterarrivalMs(double lambda)
{
    double sec;

    sec = 1.0 / lambda;
    if (sec > 10.0) {
        return MAX_INTERVAL_MS;
    }
    return round(sec * 1000.0);
}

int TimeServiceMs(double mu)
{
    double sec;

    sec = 1.0 / mu;
    if (sec > 10.0) {
        return MAX_INTERVAL_MS;
    }
    return round(sec * 1000.0);
}
