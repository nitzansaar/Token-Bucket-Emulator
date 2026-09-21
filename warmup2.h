#ifndef _WARMUP2_H_
#define _WARMUP2_H_

#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>

#include "cs402.h"

typedef struct tagArgs {
    double lambda;
    double mu;
    double r;
    int B;
    int P;
    int n;
    int use_tsfile;
    char tsfile[MAXPATHLENGTH];
} Args;

typedef struct Packet {
    int id;
    int tokens;
    int service_ms;
    struct timeval t_arrive;
} Packet;

typedef struct Shared {
    pthread_mutex_t mutex;
    pthread_cond_t cv;
    struct timeval t0;
    Args args;
    int arrived_count;
    int tokens;
    int token_count;
    int no_more_packets;
} Shared;

int ParseArgs(int argc, char **argv, Args *args);
void PrintParams(const Args *args);

void TimeNow(struct timeval *t);
void TimeElapsed(const struct timeval *t0, const struct timeval *t,
                 struct timeval *diff);
void TimeAddMs(const struct timeval *base, int ms, struct timeval *out);
void TimeAddUsec(const struct timeval *base, int usec, struct timeval *out);
void TimeSleepRemaining(const struct timeval *expected);
void TimeFormatInterval(const struct timeval *diff, char *buf, size_t n);
void TimePrintEvent(const struct timeval *t0, const struct timeval *t,
                    const char *msg);
int TimeInterarrivalMs(double lambda);
int TimeServiceMs(double mu);
int TimeTokenIntervalUsec(double r);

void *Arrival(void *arg);
void *Token(void *arg);

#endif /* _WARMUP2_H_ */
