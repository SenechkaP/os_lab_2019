#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <getopt.h>
#include <pthread.h>
#include "sumlib.h"
#include "utils.h"

struct ThreadInfo {
    pthread_t tid;
    struct SumArgs args;
};

void *ThreadSum(void *vargs) {
    struct SumArgs *args = (struct SumArgs *)vargs;
    int part_sum = Sum(args);
    return (void *)(size_t)part_sum;
}

static void usage(const char *prog) {
    printf("Usage: %s --threads_num N --seed S --array_size M\n", prog);
}

int main(int argc, char **argv) {
    uint32_t threads_num = 0;
    uint32_t array_size = 0;
    uint32_t seed = 0;

    static struct option long_options[] = {
        {"threads_num", required_argument, 0, 0},
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "", long_options, &option_index);
        if (c == -1) break;
        if (c == 0) {
            switch (option_index) {
                case 0: threads_num = (uint32_t)atoi(optarg); break;
                case 1: seed = (uint32_t)atoi(optarg); break;
                case 2: array_size = (uint32_t)atoi(optarg); break;
                default: break;
            }
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (threads_num == 0 || array_size == 0 || seed == 0) {
        usage(argv[0]);
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    if (!array) {
        perror("malloc");
        return 1;
    }
    GenerateArray(array, (int)array_size, (int)seed);

    struct ThreadInfo *tinfo = malloc(sizeof(struct ThreadInfo) * threads_num);
    if (!tinfo) {
        perror("malloc tinfo");
        free(array);
        return 1;
    }

    uint32_t base = array_size / threads_num;
    uint32_t rem = array_size % threads_num;

    struct timeval tstart, tend;
    gettimeofday(&tstart, NULL);

    for (uint32_t i = 0; i < threads_num; ++i) {
        uint32_t begin = i * base + (i < rem ? i : rem);
        uint32_t extra = (i < rem) ? 1 : 0;
        uint32_t end = begin + base + extra;

        tinfo[i].args.array = array;
        tinfo[i].args.begin = (uint32_t)begin;
        tinfo[i].args.end = (uint32_t)end;

        int rc = pthread_create(&tinfo[i].tid, NULL, ThreadSum, &tinfo[i].args);
        if (rc != 0) {
            fprintf(stderr, "Error: pthread_create failed (%d)\n", rc);
            threads_num = i;
            break;
        }
    }

    int total_sum = 0;
    for (uint32_t i = 0; i < threads_num; ++i) {
        void *ret;
        int rc = pthread_join(tinfo[i].tid, &ret);
        if (rc != 0) {
            fprintf(stderr, "Error: pthread_join failed (%d)\n", rc);
            continue;
        }
        int part_sum = (int)(size_t)ret;
        total_sum += part_sum;
    }

    gettimeofday(&tend, NULL);
    double elapsed_ms = (tend.tv_sec - tstart.tv_sec) * 1000.0 + (tend.tv_usec - tstart.tv_usec) / 1000.0;

    printf("Total: %d\n", total_sum);
    printf("Time elapsed (ms): %.3f\n", elapsed_ms);

    free(tinfo);
    free(array);
    return 0;
}
