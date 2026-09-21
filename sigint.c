#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "warmup2.h"

static void DrainQueue(Shared *s, My402List *q, int qnum)
{
    while (!My402ListEmpty(q)) {
        My402ListElem *elem = My402ListFirst(q);
        Packet *p = (Packet *)elem->obj;
        struct timeval t;
        char msg[80];

        TimeNow(&t);
        snprintf(msg, sizeof(msg), "p%d removed from Q%d", p->id, qnum);
        TimePrintEvent(&s->t0, &t, msg);
        My402ListUnlink(q, elem);
        free(p);
    }
}

void *CatchSigint(void *arg)
{
    Shared *s = (Shared *)arg;
    sigset_t set;
    int sig;
    struct timeval t;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigwait(&set, &sig);

    pthread_mutex_lock(&s->mutex);
    printf("\n");
    TimeNow(&t);
    TimePrintEvent(&s->t0, &t,
                   "SIGINT caught, no new packets or tokens will be allowed");
    s->shutdown = 1;
    s->no_more_packets = 1;
    DrainQueue(s, &s->Q1, 1);
    DrainQueue(s, &s->Q2, 2);
    pthread_cond_broadcast(&s->cv);
    pthread_mutex_unlock(&s->mutex);
    return NULL;
}
