#ifndef _WARMUP2_H_
#define _WARMUP2_H_

#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>

#include "cs402.h"
#include "my402list.h"

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
    struct timeval t_enter_q1;
    struct timeval t_leave_q1;
    struct timeval t_enter_q2;
    struct timeval t_leave_q2;
    struct timeval t_begin_svc;
    struct timeval t_depart;
} Packet;

typedef struct Stats {
    int dropped_packets;
    int dropped_tokens;
    int completed;
    double sum_ia;
    double sum_svc;
    double sum_q1;
    double sum_q2;
    double sum_s1;
    double sum_s2;
    double sum_sys;
    double sum_sys2;
} Stats;

typedef struct Shared {
    pthread_mutex_t mutex;
    pthread_cond_t cv;
    struct timeval t0;
    Args args;
    My402List Q1;
    My402List Q2;
    Stats stats;
    int arrived_count;
    int tokens;
    int token_count;
    int no_more_packets;
    int shutdown;
    FILE *tsfp;
} Shared;

typedef struct ServerArg {
    Shared *shared;
    int id;
} ServerArg;

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
double TimeToSeconds(const struct timeval *diff);
int TimeInterarrivalMs(double lambda);
int TimeServiceMs(double mu);
int TimeTokenIntervalUsec(double r);

const char *TokenWord(int n);
int TryMoveHeadQ1ToQ2(Shared *s);
int AllDone(Shared *s);

void PrintStats(Shared *s, const struct timeval *t_end);

void TsfileOpenAndReadN(Shared *s);
void TsfileReadPacket(FILE *fp, int *ia_ms, int *tokens, int *service_ms);

void *Arrival(void *arg);
void *Token(void *arg);
void *Server(void *arg);
void *CatchSigint(void *arg);

#endif /* _WARMUP2_H_ */
