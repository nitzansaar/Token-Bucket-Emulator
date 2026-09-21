#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "warmup2.h"

void *Server(void *arg)
{
    ServerArg *sa = (ServerArg *)arg;
    Shared *s = sa->shared;
    int id = sa->id;

    pthread_mutex_lock(&s->mutex);
    for (;;) {
        Packet *p;
        My402ListElem *elem;
        struct timeval q2_time;
        struct timeval svc_time;
        struct timeval sys_time;
        struct timeval wake;
        char q2_str[32];
        char svc_str[32];
        char sys_str[32];
        char msg[192];

        while (My402ListEmpty(&s->Q2) && !AllDone(s)) {
            pthread_cond_wait(&s->cv, &s->mutex);
        }
        if (My402ListEmpty(&s->Q2) && AllDone(s)) {
            pthread_mutex_unlock(&s->mutex);
            return NULL;
        }

        elem = My402ListFirst(&s->Q2);
        p = (Packet *)elem->obj;
        My402ListUnlink(&s->Q2, elem);

        TimeNow(&p->t_leave_q2);
        TimeElapsed(&p->t_enter_q2, &p->t_leave_q2, &q2_time);
        TimeFormatInterval(&q2_time, q2_str, sizeof(q2_str));
        snprintf(msg, sizeof(msg), "p%d leaves Q2, time in Q2 = %s",
                 p->id, q2_str);
        TimePrintEvent(&s->t0, &p->t_leave_q2, msg);

        TimeNow(&p->t_begin_svc);
        snprintf(msg, sizeof(msg),
                 "p%d begins service at S%d, requesting %dms of service",
                 p->id, id, p->service_ms);
        TimePrintEvent(&s->t0, &p->t_begin_svc, msg);

        if (AllDone(s)) {
            pthread_cond_broadcast(&s->cv);
        }
        pthread_mutex_unlock(&s->mutex);

        TimeNow(&wake);
        TimeAddMs(&wake, p->service_ms, &wake);
        TimeSleepRemaining(&wake);

        pthread_mutex_lock(&s->mutex);
        TimeNow(&p->t_depart);
        TimeElapsed(&p->t_begin_svc, &p->t_depart, &svc_time);
        TimeElapsed(&p->t_arrive, &p->t_depart, &sys_time);
        TimeFormatInterval(&svc_time, svc_str, sizeof(svc_str));
        TimeFormatInterval(&sys_time, sys_str, sizeof(sys_str));
        snprintf(msg, sizeof(msg),
                 "p%d departs from S%d, service time = %s, time in system = %s",
                 p->id, id, svc_str, sys_str);
        TimePrintEvent(&s->t0, &p->t_depart, msg);
        free(p);
        if (AllDone(s)) {
            pthread_cond_broadcast(&s->cv);
        }
    }
}
