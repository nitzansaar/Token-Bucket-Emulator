#include <stdio.h>
#include <stdlib.h>

#include "warmup2.h"

int main(int argc, char **argv)
{
    Args args;
    struct timeval t0;
    struct timeval t_end;

    ParseArgs(argc, argv, &args);
    PrintParams(&args);

    TimeNow(&t0);
    TimePrintEvent(&t0, &t0, "emulation begins");

    TimeNow(&t_end);
    TimePrintEvent(&t0, &t_end, "emulation ends");

    return 0;
}
