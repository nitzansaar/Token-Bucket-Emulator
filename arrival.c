#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "warmup2.h"

void *Arrival(void *arg)
{
    Shared *s = (Shared *)arg;
    int interval_ms = TimeInterarrivalMs(s->args.lambda);
    int service_ms = TimeServiceMs(s->args.mu);
    int tokens = s->args.P;
    int n = s->args.n;
    struct timeval last_actual;
    int k;

    last_actual = s->t0;

    for (k = 1; k <= n; k++) {
        struct timeval expected;
        struct timeval t_arrive;
        struct timeval ia;
        char ia_str[32];
        char msg[160];

        TimeAddMs(&last_actual, interval_ms, &expected);
        TimeSleepRemaining(&expected);

        pthread_mutex_lock(&s->mutex);
        TimeNow(&t_arrive);
        s->arrived_count++;

        TimeElapsed(&last_actual, &t_arrive, &ia);
        TimeFormatInterval(&ia, ia_str, sizeof(ia_str));
        s->stats.sum_ia += TimeToSeconds(&ia);
        if (tokens > s->args.B) {
            s->stats.dropped_packets++;
            snprintf(msg, sizeof(msg),
                     "p%d arrives, needs %d token%s, inter-arrival time = %s, dropped",
                     k, tokens, (tokens == 1) ? "" : "s", ia_str);
            TimePrintEvent(&s->t0, &t_arrive, msg);
        } else {
            Packet *pkt;
            int q1_was_empty;

            snprintf(msg, sizeof(msg),
                     "p%d arrives, needs %d token%s, inter-arrival time = %s",
                     k, tokens, (tokens == 1) ? "" : "s", ia_str);
            TimePrintEvent(&s->t0, &t_arrive, msg);

            pkt = (Packet *)malloc(sizeof(Packet));
            if (pkt == NULL) {
                perror("malloc");
                exit(1);
            }
            pkt->id = k;
            pkt->tokens = tokens;
            pkt->service_ms = service_ms;
            pkt->t_arrive = t_arrive;

            q1_was_empty = My402ListEmpty(&s->Q1);
            My402ListAppend(&s->Q1, pkt);
            TimeNow(&pkt->t_enter_q1);
            snprintf(msg, sizeof(msg), "p%d enters Q1", k);
            TimePrintEvent(&s->t0, &pkt->t_enter_q1, msg);

            if (q1_was_empty) {
                TryMoveHeadQ1ToQ2(s);
            }
        }
        last_actual = t_arrive;
        pthread_mutex_unlock(&s->mutex);
    }

    pthread_mutex_lock(&s->mutex);
    s->no_more_packets = 1;
    pthread_cond_broadcast(&s->cv);
    pthread_mutex_unlock(&s->mutex);

    return NULL;
}
