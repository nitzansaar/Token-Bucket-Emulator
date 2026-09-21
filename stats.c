#include <math.h>
#include <stdio.h>

#include "warmup2.h"

static void PrintRealOrNA(const char *label, int ok, double value,
                          const char *reason)
{
    if (ok) {
        printf("    %s = %.6g\n", label, value);
    } else {
        printf("    %s = N/A (%s)\n", label, reason);
    }
}

void PrintStats(Shared *s, const struct timeval *t_end)
{
    struct timeval total;
    double T;
    int arrived = s->arrived_count;
    int completed = s->stats.completed;
    int tokens = s->token_count;

    TimeElapsed(&s->t0, t_end, &total);
    T = TimeToSeconds(&total);

    printf("Statistics:\n\n");

    PrintRealOrNA("average packet inter-arrival time",
                  arrived > 0, (arrived > 0) ? s->stats.sum_ia / arrived : 0.0,
                  "no packet arrived");
    PrintRealOrNA("average packet service time",
                  completed > 0,
                  (completed > 0) ? s->stats.sum_svc / completed : 0.0,
                  "no packet was served");
    PrintRealOrNA("average number of packets in Q1",
                  completed > 0 && T > 0.0,
                  (T > 0.0) ? s->stats.sum_q1 / T : 0.0,
                  "no packet was served");
    PrintRealOrNA("average number of packets in Q2",
                  completed > 0 && T > 0.0,
                  (T > 0.0) ? s->stats.sum_q2 / T : 0.0,
                  "no packet was served");
    PrintRealOrNA("average number of packets at S1",
                  completed > 0 && T > 0.0,
                  (T > 0.0) ? s->stats.sum_s1 / T : 0.0,
                  "no packet was served");
    PrintRealOrNA("average number of packets at S2",
                  completed > 0 && T > 0.0,
                  (T > 0.0) ? s->stats.sum_s2 / T : 0.0,
                  "no packet was served");
    PrintRealOrNA("average time a packet spent in system",
                  completed > 0,
                  (completed > 0) ? s->stats.sum_sys / completed : 0.0,
                  "no packet was served");

    if (completed > 0) {
        double mean = s->stats.sum_sys / completed;
        double var = (s->stats.sum_sys2 / completed) - (mean * mean);
        if (var < 0.0) {
            var = 0.0;
        }
        PrintRealOrNA("standard deviation for time spent in system",
                      1, sqrt(var), "no packet was served");
    } else {
        PrintRealOrNA("standard deviation for time spent in system",
                      0, 0.0, "no packet was served");
    }

    PrintRealOrNA("token drop probability",
                  tokens > 0,
                  (tokens > 0) ? (double)s->stats.dropped_tokens / tokens : 0.0,
                  "no token was produced");
    PrintRealOrNA("packet drop probability",
                  arrived > 0,
                  (arrived > 0) ? (double)s->stats.dropped_packets / arrived : 0.0,
                  "no packet arrived");
}
