#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "warmup2.h"

#define INT_MAX_ALLOWED 2147483647

static void Usage(void)
{
    fprintf(stderr,
            "usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] "
            "[-P P] [-n num] [-t tsfile]\n");
}

static void DieUsage(const char *msg)
{
    if (msg != NULL) {
        fprintf(stderr, "%s\n", msg);
    }
    Usage();
    exit(1);
}

static int IsOption(const char *s)
{
    return (s != NULL && s[0] == '-' && s[1] != '\0');
}

static const char *NeedValue(int argc, char **argv, int i, const char *flag)
{
    if (i + 1 >= argc || IsOption(argv[i + 1])) {
        fprintf(stderr, "missing value for %s\n", flag);
        Usage();
        exit(1);
    }
    return argv[i + 1];
}

static double ParsePositiveReal(const char *flag, const char *s)
{
    char *end = NULL;
    double v;

    errno = 0;
    v = strtod(s, &end);
    if (end == s || *end != '\0' || errno == ERANGE || v <= 0.0) {
        fprintf(stderr, "invalid %s value: %s\n", flag, s);
        Usage();
        exit(1);
    }
    return v;
}

static int ParsePositiveInt(const char *flag, const char *s)
{
    char *end = NULL;
    long v;

    errno = 0;
    v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || errno == ERANGE ||
        v <= 0 || v > INT_MAX_ALLOWED) {
        fprintf(stderr, "invalid %s value: %s\n", flag, s);
        Usage();
        exit(1);
    }
    return (int)v;
}

void PrintParams(const Args *args)
{
    printf("Emulation Parameters:\n");
    if (!args->use_tsfile) {
        printf("    number to arrive = %d\n", args->n);
        printf("    lambda = %.6g\n", args->lambda);
        printf("    mu = %.6g\n", args->mu);
    }
    printf("    r = %.6g\n", args->r);
    printf("    B = %d\n", args->B);
    if (!args->use_tsfile) {
        printf("    P = %d\n", args->P);
    } else {
        printf("    tsfile = %s\n", args->tsfile);
    }
}

int ParseArgs(int argc, char **argv, Args *args)
{
    int i;

    args->lambda = 1.0;
    args->mu = 0.35;
    args->r = 1.5;
    args->B = 10;
    args->P = 3;
    args->n = 20;
    args->use_tsfile = FALSE;
    args->tsfile[0] = '\0';

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-lambda") == 0) {
            args->lambda = ParsePositiveReal(argv[i],
                                             NeedValue(argc, argv, i, argv[i]));
            i++;
        } else if (strcmp(argv[i], "-mu") == 0) {
            args->mu = ParsePositiveReal(argv[i],
                                         NeedValue(argc, argv, i, argv[i]));
            i++;
        } else if (strcmp(argv[i], "-r") == 0) {
            args->r = ParsePositiveReal(argv[i],
                                        NeedValue(argc, argv, i, argv[i]));
            i++;
        } else if (strcmp(argv[i], "-B") == 0) {
            args->B = ParsePositiveInt(argv[i],
                                       NeedValue(argc, argv, i, argv[i]));
            i++;
        } else if (strcmp(argv[i], "-P") == 0) {
            args->P = ParsePositiveInt(argv[i],
                                       NeedValue(argc, argv, i, argv[i]));
            i++;
        } else if (strcmp(argv[i], "-n") == 0) {
            args->n = ParsePositiveInt(argv[i],
                                       NeedValue(argc, argv, i, argv[i]));
            i++;
        } else if (strcmp(argv[i], "-t") == 0) {
            const char *path = NeedValue(argc, argv, i, argv[i]);
            if (strlen(path) >= MAXPATHLENGTH) {
                DieUsage("tsfile path is too long");
            }
            strncpy(args->tsfile, path, MAXPATHLENGTH - 1);
            args->tsfile[MAXPATHLENGTH - 1] = '\0';
            args->use_tsfile = TRUE;
            i++;
        } else {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            Usage();
            exit(1);
        }
    }

    return TRUE;
}
