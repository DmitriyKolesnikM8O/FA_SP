#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <string.h>

int N;
sem_t *forks;  sem_t mutex;   
int meals_count = 10;  
int use_synchronization = 1;  


void* dining_person(void* num) {
    int id = *((int*)num);
    int left_fork = id;
    int right_fork = (id + 1) % N;
    int meals_eaten = 0;

    while (meals_eaten < meals_count) {
        printf("Philosopher %d is thinking... (meals left: %d)\n", id + 1, meals_count - meals_eaten);
        usleep((rand() % 500 + 100) * 1000); 

        if (use_synchronization) {
            sem_wait(&mutex);  
        }

        sem_wait(&forks[left_fork]);
        printf("Philosopher %d picked up left fork %d\n", id + 1, left_fork + 1);

        
        sem_wait(&forks[right_fork]);
        printf("Philosopher %d picked up right fork %d\n", id + 1, right_fork + 1);

        if (use_synchronization) {
            sem_post(&mutex);  
        }

        printf("Philosopher %d is eating... (meals left: %d)\n", id + 1, meals_count - meals_eaten - 1);
        usleep((rand() % 500 + 200) * 1000); 
        meals_eaten++;

        sem_post(&forks[left_fork]);
        printf("Philosopher %d put down left fork %d\n", id + 1, left_fork + 1);
        sem_post(&forks[right_fork]);
        printf("Philosopher %d put down right fork %d\n", id + 1, right_fork + 1);
    }

    printf("Philosopher %d has finished dining.\n", id + 1);
    return NULL;
}


int main(int argc, char *argv[]) {
    srand(time(NULL));

    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: %s <N> [nosync]\n", argv[0]);
        fprintf(stderr, "  <N> - number of philosophers\n");
        fprintf(stderr, "  nosync - disable synchronization to demonstrate deadlock\n");
        return 1;
    }

    char *endptr;
    long N_long = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || N_long <= 1 || N_long > INT_MAX) {
        fprintf(stderr, "Error: N must be an integer > 1 and < %d\n", INT_MAX);
        return 1;
    }
    N = (int)N_long;

    if (argc == 3 && strcmp(argv[2], "nosync") == 0) {
        use_synchronization = 0;
        printf("Running without synchronization — deadlock possible!\n");
    } else {
        printf("Running with synchronization (mutex = %d).\n", N - 1);
    }

    // Allocate memory
    forks = malloc(N * sizeof(sem_t));
    if (!forks) {
        fprintf(stderr, "Error: Failed to allocate memory for forks.\n");
        return 1;
    }

    pthread_t *philosophers = malloc(N * sizeof(pthread_t));
    int *ids = malloc(N * sizeof(int));
    if (!philosophers || !ids) {
        fprintf(stderr, "Error: Failed to allocate memory for threads.\n");
        free(forks);
        free(philosophers);
        free(ids);
        return 1;
    }

    // Initialize semaphores with error tracking
    int i;
    int success = 1;
    for (i = 0; i < N && success; i++) {
        if (sem_init(&forks[i], 0, 1) != 0) {
            fprintf(stderr, "Error: Failed to initialize semaphore %d.\n", i);
            success = 0;
        }
    }

    if (success && sem_init(&mutex, 0, N - 1) != 0) {
        fprintf(stderr, "Error: Failed to initialize mutex semaphore.\n");
        success = 0;
    }

    if (!success) {
        // Clean up initialized semaphores before exiting
        for (int j = 0; j < i; j++) {
            sem_destroy(&forks[j]);
        }
        free(forks);
        free(philosophers);
        free(ids);
        return 1;
    }

    // Create threads
    for (i = 0; i < N && success; i++) {
        ids[i] = i;
        if (pthread_create(&philosophers[i], NULL, dining_person, &ids[i]) != 0) {
            fprintf(stderr, "Error: Failed to create thread for philosopher %d.\n", i + 1);
            success = 0;
        }
    }

    if (success) {
        // Wait for threads to finish
        for (i = 0; i < N; i++) {
            pthread_join(philosophers[i], NULL);
        }
        printf("All philosophers have finished dining.\n");
    }

    // Free resources
    for (i = 0; i < N; i++) {
        sem_destroy(&forks[i]);
    }
    sem_destroy(&mutex);
    free(forks);
    free(philosophers);
    free(ids);

    return success ? 0 : 1;
}