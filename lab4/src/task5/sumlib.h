#ifndef SUMLIB_H
#define SUMLIB_H

#include <stdint.h>

struct SumArgs {
    int *array;
    uint32_t begin;
    uint32_t end;
};

int Sum(const struct SumArgs *args);

#endif
