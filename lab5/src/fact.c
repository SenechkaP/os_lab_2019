#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <stdint.h>
#include <inttypes.h>

typedef unsigned long long ull;

pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
ull global_result = 1;
ull mod_value = 1;

typedef struct {
    ull start;
    ull end;
} range_t;

static inline ull modmul(ull a, ull b, ull mod) {
    if (mod == 0) return 0;
    __int128 t = (__int128)a * (__int128)b;
    return (ull)(t % mod);
}

void *worker(void *arg) {
    range_t *r = (range_t *)arg;
    ull local = 1;

    if (mod_value == 1) {
        local = 0;
    } else {
        for (ull i = r->start; i <= r->end; ++i) {
            local = modmul(local, i % mod_value, mod_value);
            if (local == 0) {
                break;
            }
        }
    }

    pthread_mutex_lock(&result_mutex);
    if (mod_value == 1) {
        global_result = 0;
    } else {
        global_result = modmul(global_result, local, mod_value);
    }
    pthread_mutex_unlock(&result_mutex);

    return NULL;
}

void print_usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s -k K -p PNUM -m MOD\n"
            "  -k, --k        K (compute K!)\n"
            "  -p, --pnum     number of threads\n"
            "  -m, --mod      modulus (positive integer)\n"
            "Example: %s -k 10 --pnum=4 --mod=1000\n",
            prog, prog);
}

int main(int argc, char **argv) {
    long long k = -1;
    int pnum = -1;
    long long mod = -1;

    const struct option longopts[] = {
        {"k", required_argument, NULL, 'k'},
        {"pnum", required_argument, NULL, 'p'},
        {"mod", required_argument, NULL, 'm'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "k:p:m:", longopts, NULL)) != -1) {
        switch (opt) {
            case 'k': k = atoll(optarg); break;
            case 'p': pnum = atoi(optarg); break;
            case 'm': mod = atoll(optarg); break;
            default: print_usage(argv[0]); return EXIT_FAILURE;
        }
    }

    if (k < 0 || pnum <= 0 || mod <= 0) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    mod_value = (ull)mod;

    if (k == 0) {
        printf("Result: %llu\n", (ull)(1 % mod_value));
        return 0;
    }

    if ((long long)pnum > k) pnum = (int)k;

    pthread_t *threads = malloc(sizeof(pthread_t) * pnum);
    range_t *ranges = malloc(sizeof(range_t) * pnum);
    if (threads == NULL || ranges == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    ull base = (ull)k / (ull)pnum;
    ull rem = (ull)k % (ull)pnum;
    ull cur = 1;

    for (int i = 0; i < pnum; ++i) {
        ull len = base + (i < (int)rem ? 1 : 0);
        ranges[i].start = cur;
        ranges[i].end = cur + len - 1;
        cur += len;
    }

    for (int i = 0; i < pnum; ++i) {
        if (pthread_create(&threads[i], NULL, worker, &ranges[i]) != 0) {
            perror("pthread_create");
            for (int j = 0; j < i; ++j) pthread_join(threads[j], NULL);
            free(threads); free(ranges);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < pnum; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("Result: %llu\n", (ull)(global_result % mod_value));

    free(threads);
    free(ranges);
    pthread_mutex_destroy(&result_mutex);
    return 0;
}
