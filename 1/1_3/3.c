#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <string.h>

int N;
sem_t *forks;  // Динамический массив семафоров для вилок
sem_t mutex;   // Семафор для предотвращения deadlock
int meals_count = 10;  // Количество приемов пищи
int use_synchronization = 1;  // Флаг для включения/выключения синхронизации

void* philosopher(void* num) {
    int id = *((int*)num);
    int left_fork = id;
    int right_fork = (id + 1) % N;
    int meals_eaten = 0;

    while (meals_eaten < meals_count) {
        printf("Философ %d думает... (осталось еды: %d)\n", id + 1, meals_count - meals_eaten);
        usleep((rand() % 500 + 100) * 1000); // Случайное время мышления: 100-600 мс

        if (use_synchronization) {
            sem_wait(&mutex);  // Ограничиваем число философов, берущих вилки
        }

        sem_wait(&forks[left_fork]);
        printf("Философ %d взял левую вилку %d\n", id + 1, left_fork + 1);

        // Симуляция deadlock без синхронизации: все берут левую вилку и ждут правую
        sem_wait(&forks[right_fork]);
        printf("Философ %d взял правую вилку %d\n", id + 1, right_fork + 1);

        if (use_synchronization) {
            sem_post(&mutex);  // Освобождаем доступ для других
        }

        printf("Философ %d ест... (осталось еды: %d)\n", id + 1, meals_count - meals_eaten - 1);
        usleep((rand() % 500 + 200) * 1000); // Случайное время еды: 200-700 мс
        meals_eaten++;

        sem_post(&forks[left_fork]);
        printf("Философ %d положил левую вилку %d\n", id + 1, left_fork + 1);
        sem_post(&forks[right_fork]);
        printf("Философ %d положил правую вилку %d\n", id + 1, right_fork + 1);
    }

    printf("Философ %d завершил трапезу.\n", id + 1);
    return NULL;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: %s <N> [nosync]\n", argv[0]);
        fprintf(stderr, "  <N> - number of philosophers\n");
        fprintf(stderr, "  nosync - disable synchronization to show deadlock\n");
        return 1;
    }

    char *endptr;
    long N_long = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || N_long <= 1 || N_long > INT_MAX) {
        fprintf(stderr, "Ошибка: N должно быть целым числом > 1 и < %d\n", INT_MAX);
        return 1;
    }
    N = (int)N_long;

    if (argc == 3 && strcmp(argv[2], "nosync") == 0) {
        use_synchronization = 0;
        printf("Запуск без синхронизации — возможен deadlock!\n");
    } else {
        printf("Запуск с синхронизацией (mutex = %d).\n", N - 1);
    }

    // Выделение памяти
    forks = malloc(N * sizeof(sem_t));
    if (!forks) {
        fprintf(stderr, "Ошибка: не удалось выделить память для вилок.\n");
        return 1;
    }

    pthread_t *philosophers = malloc(N * sizeof(pthread_t));
    int *ids = malloc(N * sizeof(int));
    if (!philosophers || !ids) {
        fprintf(stderr, "Ошибка: не удалось выделить память для потоков.\n");
        free(forks);
        free(philosophers);
        free(ids);
        return 1;
    }

    // Инициализация семафоров
    for (int i = 0; i < N; i++) {
        if (sem_init(&forks[i], 0, 1) != 0) {
            fprintf(stderr, "Ошибка: не удалось инициализировать семафор %d.\n", i);
            goto cleanup;
        }
    }
    if (sem_init(&mutex, 0, N - 1) != 0) {
        fprintf(stderr, "Ошибка: не удалось инициализировать семафор mutex.\n");
        goto cleanup;
    }

    // Создание потоков
    for (int i = 0; i < N; i++) {
        ids[i] = i;
        if (pthread_create(&philosophers[i], NULL, philosopher, &ids[i]) != 0) {
            fprintf(stderr, "Ошибка: не удалось создать поток для философа %d.\n", i + 1);
            goto cleanup;
        }
    }

    // Ожидание завершения
    for (int i = 0; i < N; i++) {
        pthread_join(philosophers[i], NULL);
    }

    printf("Все философы завершили трапезу.\n");

cleanup:
    // Освобождение ресурсов
    for (int i = 0; i < N; i++) {
        sem_destroy(&forks[i]);
    }
    sem_destroy(&mutex);
    free(forks);
    free(philosophers);
    free(ids);
    return 0;
}