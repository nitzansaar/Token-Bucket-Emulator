#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "warmup2.h"

static int TokenShouldStop(Shared *s)
{
    return (s->shutdown || (s->no_more_packets && My402ListEmpty(&s->Q1)));
}

void *Token(void *arg)
{
    Shared *s = (Shared *)arg;
    int interval_usec = TimeTokenIntervalUsec(s->args.r);
    struct timeval last_actual;
    int id = 0;

    last_actual = s->t0;

    for (;;) {
        struct timeval expected;
        struct timeval t;
        char msg[160];

        pthread_mutex_lock(&s->mutex);
        if (TokenShouldStop(s)) {
            pthread_mutex_unlock(&s->mutex);
            return NULL;
        }
        pthread_mutex_unlock(&s->mutex);

        TimeAddUsec(&last_actual, interval_usec, &expected);
        TimeSleepRemaining(&expected);

        pthread_mutex_lock(&s->mutex);
        if (TokenShouldStop(s)) {
            pthread_mutex_unlock(&s->mutex);
            return NULL;
        }

        TimeNow(&t);
        id++;
        s->token_count++;
        if (s->tokens < s->args.B) {
            s->tokens++;
            snprintf(msg, sizeof(msg),
                     "token t%d arrives, token bucket now has %d %s",
                     id, s->tokens, TokenWord(s->tokens));
        } else {
            s->stats.dropped_tokens++;
            snprintf(msg, sizeof(msg), "token t%d arrives, dropped", id);
        }
        TimePrintEvent(&s->t0, &t, msg);
        last_actual = t;

        TryMoveHeadQ1ToQ2(s);
        pthread_mutex_unlock(&s->mutex);
    }
}
