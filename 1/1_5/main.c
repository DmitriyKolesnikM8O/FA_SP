#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>

pthread_mutex_t mutex;
int state = 0;                // 0 - никого нет, 1 - только женщины, 2 - только мужчины
int current_count = 0;
int N;
pthread_cond_t cond_woman;
pthread_cond_t cond_man;

void woman_wants_to_enter() {
    pthread_mutex_lock(&mutex);
    while (state == 2 || current_count >= N) {
        pthread_cond_wait(&cond_woman, &mutex);
    }
    current_count++;
    state = 1;  // В ванной только женщины
    printf("Woman entered. People in bathroom now: %d\n", current_count);
    pthread_mutex_unlock(&mutex);
}

void man_wants_to_enter() {
    pthread_mutex_lock(&mutex);
    while (state == 1 || current_count >= N) {
        pthread_cond_wait(&cond_man, &mutex);
    }
    current_count++;
    state = 2;
    printf("Man entered. People in bathroom now: %d\n", current_count);
    pthread_mutex_unlock(&mutex);
}

void woman_leaves() {
    pthread_mutex_lock(&mutex);
    current_count--;
    if (current_count == 0) {
        state = 0;
        pthread_cond_broadcast(&cond_man);
        pthread_cond_broadcast(&cond_woman);
    }
    printf("Woman left. People in bathroom now: %d\n", current_count);
    pthread_mutex_unlock(&mutex);
}

void man_leaves() {
    pthread_mutex_lock(&mutex);
    current_count--;
    if (current_count == 0) {
        state = 0;
        pthread_cond_broadcast(&cond_man);
        pthread_cond_broadcast(&cond_woman);
    }
    printf("Man left. People in bathroom now: %d\n", current_count);
    pthread_mutex_unlock(&mutex);
}

void *man_thread() {
    man_wants_to_enter();
    sleep(1);
    man_leaves();
    return NULL;
}

void *woman_thread() {
    woman_wants_to_enter();
    sleep(1);
    woman_leaves();
    return NULL;
}

typedef enum statusCode {
    SUCCESS,
    INVALID_ARGS
} statusCode;


statusCode str_to_int(const char *str, int *res) {
    if (!str || !res) {
        return INVALID_ARGS;
    }

    char *endptr;
    long tmp = strtol(str, &endptr, 10);

    if (*endptr != '\0') {
        return INVALID_ARGS;
    }

    if (tmp < 0 || tmp > INT_MAX) {
        return INVALID_ARGS;
    }

    *res = (int) tmp;
    return SUCCESS;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Use %s [N]\n", argv[0]);
        return INVALID_ARGS;
    }

    statusCode err = str_to_int(argv[1], &N);
    if (err != SUCCESS) {
        fprintf(stderr, "N must be int > 0\n");
        return INVALID_ARGS;
    }

    pthread_t threads[10];


    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_woman, NULL);
    pthread_cond_init(&cond_man, NULL);


    for (int i = 0; i < 5; i++) {
        pthread_create(&threads[i], NULL, woman_thread, NULL);  // Потоки для женщин
        pthread_create(&threads[5 + i], NULL, man_thread, NULL);  // Потоки для мужчин
    }


    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_man);
    pthread_cond_destroy(&cond_woman);
    return SUCCESS;
}