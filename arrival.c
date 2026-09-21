#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "warmup2.h"

#define TS_LINE_MAX 1024
#define INT_MAX_ALLOWED 2147483647

static void DieTsfile(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static void ReadTsLine(FILE *fp, char *buf)
{
    size_t len;

    if (fgets(buf, TS_LINE_MAX + 1, fp) == NULL) {
        DieTsfile("malformed tsfile: unexpected end of file");
    }
    len = strlen(buf);
    if (len == (size_t)TS_LINE_MAX && buf[TS_LINE_MAX - 1] != '\n') {
        DieTsfile("malformed tsfile: line too long");
    }
    if (buf[0] == ' ' || buf[0] == '\t') {
        DieTsfile("malformed tsfile: leading whitespace");
    }
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
    }
    if (len > 0 && (buf[len - 1] == ' ' || buf[len - 1] == '\t')) {
        DieTsfile("malformed tsfile: trailing whitespace");
    }
}

static int ParsePositiveIntToken(char **pp)
{
    char *s = *pp;
    char *end = NULL;
    long v;

    errno = 0;
    v = strtol(s, &end, 10);
    if (end == s || errno == ERANGE || v <= 0 || v > INT_MAX_ALLOWED) {
        DieTsfile("malformed tsfile: invalid integer");
    }
    *pp = end;
    return (int)v;
}

static void SkipSep(char **pp)
{
    if (**pp != ' ' && **pp != '\t') {
        DieTsfile("malformed tsfile: missing field separator");
    }
    while (**pp == ' ' || **pp == '\t') {
        (*pp)++;
    }
}

void TsfileOpenAndReadN(Shared *s)
{
    char buf[TS_LINE_MAX + 1];
    char *p;
    int n;

    s->tsfp = fopen(s->args.tsfile, "r");
    if (s->tsfp == NULL) {
        fprintf(stderr, "cannot open tsfile: %s\n", s->args.tsfile);
        exit(1);
    }
    ReadTsLine(s->tsfp, buf);
    p = buf;
    n = ParsePositiveIntToken(&p);
    if (*p != '\0') {
        DieTsfile("malformed tsfile: extra junk on first line");
    }
    s->args.n = n;
}

void TsfileReadPacket(FILE *fp, int *ia_ms, int *tokens, int *service_ms)
{
    char buf[TS_LINE_MAX + 1];
    char *p;

    ReadTsLine(fp, buf);
    p = buf;
    *ia_ms = ParsePositiveIntToken(&p);
    SkipSep(&p);
    *tokens = ParsePositiveIntToken(&p);
    SkipSep(&p);
    *service_ms = ParsePositiveIntToken(&p);
    if (*p != '\0') {
        DieTsfile("malformed tsfile: extra junk on packet line");
    }
}

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

        if (s->args.use_tsfile) {
            TsfileReadPacket(s->tsfp, &interval_ms, &tokens, &service_ms);
        }

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
    if (s->tsfp != NULL) {
        fclose(s->tsfp);
        s->tsfp = NULL;
    }
    pthread_mutex_unlock(&s->mutex);

    return NULL;
}
