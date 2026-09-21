#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "warmup2.h"
#include "my402list.h"

const char *TokenWord(int n)
{
    return (n == 1 || n == 0) ? "token" : "tokens";
}

int TryMoveHeadQ1ToQ2(Shared *s)
{
    My402ListElem *elem;
    Packet *p;
    struct timeval q1_time;
    char q1_str[32];
    char msg[160];

    if (My402ListEmpty(&s->Q1)) {
        return 0;
    }
    elem = My402ListFirst(&s->Q1);
    p = (Packet *)elem->obj;
    if (s->tokens < p->tokens) {
        return 0;
    }

    s->tokens -= p->tokens;
    TimeNow(&p->t_leave_q1);
    TimeElapsed(&p->t_enter_q1, &p->t_leave_q1, &q1_time);
    TimeFormatInterval(&q1_time, q1_str, sizeof(q1_str));
    snprintf(msg, sizeof(msg),
             "p%d leaves Q1, time in Q1 = %s, token bucket now has %d %s",
             p->id, q1_str, s->tokens, TokenWord(s->tokens));
    TimePrintEvent(&s->t0, &p->t_leave_q1, msg);

    My402ListUnlink(&s->Q1, elem);
    My402ListAppend(&s->Q2, p);
    TimeNow(&p->t_enter_q2);
    snprintf(msg, sizeof(msg), "p%d enters Q2", p->id);
    TimePrintEvent(&s->t0, &p->t_enter_q2, msg);
    pthread_cond_broadcast(&s->cv);
    return 1;
}

int AllDone(Shared *s)
{
    return (s->no_more_packets &&
            My402ListEmpty(&s->Q1) &&
            My402ListEmpty(&s->Q2));
}

int main(int argc, char **argv)
{
    Shared shared;
    pthread_t arrival_thr;
    pthread_t token_thr;
    pthread_t s1_thr;
    pthread_t s2_thr;
    pthread_t sigint_thr;
    ServerArg s1_arg;
    ServerArg s2_arg;
    struct timeval t_end;
    sigset_t set;

    ParseArgs(argc, argv, &shared.args);
    shared.tsfp = NULL;
    shared.arrived_count = 0;
    shared.tokens = 0;
    shared.token_count = 0;
    shared.no_more_packets = 0;
    shared.shutdown = 0;
    memset(&shared.stats, 0, sizeof(shared.stats));
    My402ListInit(&shared.Q1);
    My402ListInit(&shared.Q2);
    if (pthread_mutex_init(&shared.mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(1);
    }
    if (pthread_cond_init(&shared.cv, NULL) != 0) {
        perror("pthread_cond_init");
        exit(1);
    }

    if (shared.args.use_tsfile) {
        TsfileOpenAndReadN(&shared);
    }
    PrintParams(&shared.args);

    pthread_mutex_lock(&shared.mutex);
    TimeNow(&shared.t0);
    TimePrintEvent(&shared.t0, &shared.t0, "emulation begins");
    pthread_mutex_unlock(&shared.mutex);

    signal(SIGINT, SIG_DFL);
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("pthread_sigmask");
        exit(1);
    }

    if (pthread_create(&sigint_thr, NULL, CatchSigint, &shared) != 0) {
        perror("pthread_create");
        exit(1);
    }
    if (pthread_create(&arrival_thr, NULL, Arrival, &shared) != 0) {
        perror("pthread_create");
        exit(1);
    }
    if (pthread_create(&token_thr, NULL, Token, &shared) != 0) {
        perror("pthread_create");
        exit(1);
    }
    s1_arg.shared = &shared;
    s1_arg.id = 1;
    s2_arg.shared = &shared;
    s2_arg.id = 2;
    if (pthread_create(&s1_thr, NULL, Server, &s1_arg) != 0) {
        perror("pthread_create");
        exit(1);
    }
    if (pthread_create(&s2_thr, NULL, Server, &s2_arg) != 0) {
        perror("pthread_create");
        exit(1);
    }
    pthread_join(arrival_thr, NULL);
    pthread_join(token_thr, NULL);
    pthread_join(s1_thr, NULL);
    pthread_join(s2_thr, NULL);
    pthread_cancel(sigint_thr);
    pthread_join(sigint_thr, NULL);

    pthread_mutex_lock(&shared.mutex);
    TimeNow(&t_end);
    TimePrintEvent(&shared.t0, &t_end, "emulation ends");
    PrintStats(&shared, &t_end);
    pthread_mutex_unlock(&shared.mutex);

    pthread_cond_destroy(&shared.cv);
    pthread_mutex_destroy(&shared.mutex);
    return 0;
}
