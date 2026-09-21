#ifndef _WARMUP2_H_
#define _WARMUP2_H_

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

int ParseArgs(int argc, char **argv, Args *args);
void PrintParams(const Args *args);

void TimeNow(struct timeval *t);
void TimeElapsed(const struct timeval *t0, const struct timeval *t,
                 struct timeval *diff);
void TimePrintEvent(const struct timeval *t0, const struct timeval *t,
                    const char *msg);

#endif /* _WARMUP2_H_ */
