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
        Packet pkt;
        struct timeval expected;
        struct timeval ia;
        char ia_str[32];
        char msg[160];

        TimeAddMs(&last_actual, interval_ms, &expected);
        TimeSleepRemaining(&expected);

        pthread_mutex_lock(&s->mutex);
        TimeNow(&pkt.t_arrive);
        pkt.id = k;
        pkt.tokens = tokens;
        pkt.service_ms = service_ms;
        s->arrived_count++;

        TimeElapsed(&last_actual, &pkt.t_arrive, &ia);
        TimeFormatInterval(&ia, ia_str, sizeof(ia_str));
        if (tokens > s->args.B) {
            snprintf(msg, sizeof(msg),
                     "p%d arrives, needs %d token%s, inter-arrival time = %s, dropped",
                     k, tokens, (tokens == 1) ? "" : "s", ia_str);
        } else {
            snprintf(msg, sizeof(msg),
                     "p%d arrives, needs %d token%s, inter-arrival time = %s",
                     k, tokens, (tokens == 1) ? "" : "s", ia_str);
        }
        TimePrintEvent(&s->t0, &pkt.t_arrive, msg);
        last_actual = pkt.t_arrive;
        pthread_mutex_unlock(&s->mutex);
    }

    return NULL;
}
