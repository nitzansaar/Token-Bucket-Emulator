#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "warmup2.h"

int main(int argc, char **argv)
{
    Shared shared;
    pthread_t arrival_thr;
    struct timeval t_end;

    ParseArgs(argc, argv, &shared.args);
    PrintParams(&shared.args);

    shared.arrived_count = 0;
    if (pthread_mutex_init(&shared.mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(1);
    }

    pthread_mutex_lock(&shared.mutex);
    TimeNow(&shared.t0);
    TimePrintEvent(&shared.t0, &shared.t0, "emulation begins");
    pthread_mutex_unlock(&shared.mutex);

    if (pthread_create(&arrival_thr, NULL, Arrival, &shared) != 0) {
        perror("pthread_create");
        exit(1);
    }
    pthread_join(arrival_thr, NULL);

    pthread_mutex_lock(&shared.mutex);
    TimeNow(&t_end);
    TimePrintEvent(&shared.t0, &t_end, "emulation ends");
    pthread_mutex_unlock(&shared.mutex);

    pthread_mutex_destroy(&shared.mutex);
    return 0;
}
