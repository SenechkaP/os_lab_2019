#include "sumlib.h"

int Sum(const struct SumArgs *args) {
    int sum = 0;
    if (!args || !args->array) return 0;
    for (uint32_t i = args->begin; i < args->end; ++i) {
        sum += args->array[i];
    }
    return sum;
}
