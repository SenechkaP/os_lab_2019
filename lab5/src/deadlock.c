#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void *thread_a(void *arg) {
    (void)arg;
    printf("A: пытаюсь захватить m1\n");
    pthread_mutex_lock(&m1);
    printf("A: захватил m1\n");

    sleep(1);

    printf("A: пытаюсь захватить m2\n");
    pthread_mutex_lock(&m2);
    printf("A: захватил m2 (этого не увидим при deadlock)\n");

    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    return NULL;
}

void *thread_b(void *arg) {
    (void)arg;
    printf("B: пытаюсь захватить m2\n");
    pthread_mutex_lock(&m2);
    printf("B: захватил m2\n");

    sleep(1);

    printf("B: пытаюсь захватить m1\n");
    pthread_mutex_lock(&m1);
    printf("B: захватил m1\n");

    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);
    return NULL;
}

int main(void) {
    pthread_t a, b;

    if (pthread_create(&a, NULL, thread_a, NULL) != 0) {
        perror("pthread_create A");
        exit(1);
    }
    if (pthread_create(&b, NULL, thread_b, NULL) != 0) {
        perror("pthread_create B");
        exit(1);
    }

    pthread_join(a, NULL);
    pthread_join(b, NULL);

    printf("Оба потока завершились\n");
    return 0;
}
