#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>


pthread_mutex_t mutex;
pthread_cond_t cond_women;
pthread_cond_t cond_men;
int women_count = 0;  
int men_count = 0;    
int max_capacity;     
volatile int running = 1; 

enum bathroom_state {
    EMPTY,
    WOMEN_ONLY,
    MEN_ONLY
};


enum bathroom_state get_state() {
    if (women_count == 0 && men_count == 0) return EMPTY;
    if (women_count > 0) return WOMEN_ONLY;
    return MEN_ONLY;
}


void print_state(const char *action) {
    enum bathroom_state state = get_state();
    printf("%s: ", action);
    switch (state) {
        case EMPTY: printf("Empty"); break;
        case WOMEN_ONLY: printf("Women only (%d)", women_count); break;
        case MEN_ONLY: printf("Men only (%d)", men_count); break;
    }
    printf(" | Total: %d\n", women_count + men_count);
    fflush(stdout);
}

void woman_wants_to_enter() {
    pthread_mutex_lock(&mutex);
    while (men_count > 0 || (women_count + men_count >= max_capacity)) {
        printf("Woman waiting... (Men: %d, Total: %d)\n", men_count, women_count + men_count);
        pthread_cond_wait(&cond_women, &mutex);
    }
    women_count++;
    print_state("Woman enters");
    pthread_mutex_unlock(&mutex);
    pthread_cond_broadcast(&cond_men);
    pthread_cond_broadcast(&cond_women);
}

void man_wants_to_enter() {
    pthread_mutex_lock(&mutex);
    while (women_count > 0 || (women_count + men_count >= max_capacity)) {
        printf("Man waiting... (Women: %d, Total: %d)\n", women_count, women_count + men_count);
        pthread_cond_wait(&cond_men, &mutex);
    }
    men_count++;
    print_state("Man enters");
    pthread_mutex_unlock(&mutex);
    pthread_cond_broadcast(&cond_women);
    pthread_cond_broadcast(&cond_men);
}

void woman_leaves() {
    pthread_mutex_lock(&mutex);
    if (women_count > 0) {
        women_count--;
        print_state("Woman leaves");
        pthread_cond_broadcast(&cond_men);
        pthread_cond_broadcast(&cond_women);
    }
    pthread_mutex_unlock(&mutex);
}

void man_leaves() {
    pthread_mutex_lock(&mutex);
    if (men_count > 0) {
        men_count--;
        print_state("Man leaves");
        pthread_cond_broadcast(&cond_women);
        pthread_cond_broadcast(&cond_men);
    }
    pthread_mutex_unlock(&mutex);
}


void* woman_thread() {
    int attempts = rand() % 3 + 1;
    for (int i = 0; i < attempts && running; i++) {
        usleep((rand() % 500) * 1000);
        woman_wants_to_enter();
        usleep((rand() % 1000 + 500) * 1000);
        woman_leaves();
        usleep((rand() % 1000) * 1000);
    }
    return NULL;
}


void* man_thread() {
    int attempts = rand() % 3 + 1;
    for (int i = 0; i < attempts && running; i++) {
        usleep((rand() % 500) * 1000);
        man_wants_to_enter();
        usleep((rand() % 1000 + 500) * 1000); 
        man_leaves();
        usleep((rand() % 1000) * 1000); 
    }
    return NULL;
}

typedef enum {
    SUCCESS,
    INVALID_ARGS,
    THREAD_ERROR,
    INIT_ERROR
} statusCode;

statusCode str_to_int(const char *str, int *res) {
    if (!str || !res) return INVALID_ARGS;
    char *endptr;
    long tmp = strtol(str, &endptr, 10);
    if (*endptr != '\0' || tmp <= 0 || tmp > INT_MAX) return INVALID_ARGS;
    *res = (int)tmp;
    return SUCCESS;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <N>\n", argv[0]);
        return INVALID_ARGS;
    }

    statusCode err = str_to_int(argv[1], &max_capacity);
    if (err != SUCCESS) {
        fprintf(stderr, "N must be a positive integer\n");
        return INVALID_ARGS;
    }

   
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        printf("Failed to initialize mutex");
        return INIT_ERROR;
    }
    if (pthread_cond_init(&cond_women, NULL) != 0) {
        printf("Failed to initialize women condition");
        pthread_mutex_destroy(&mutex);
        return INIT_ERROR;
    }
    if (pthread_cond_init(&cond_men, NULL) != 0) {
        printf("Failed to initialize men condition");
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond_women);
        return INIT_ERROR;
    }

    srand(time(NULL));
    const int total_threads = max_capacity + 2; 
    pthread_t threads[total_threads];
    int women_threads = rand() % (total_threads / 2 + 1) + 1;
    int men_threads = total_threads - women_threads; 

    printf("Starting with %d women and %d men, max capacity = %d\n", women_threads, men_threads, max_capacity);

    
    for (int i = 0; i < women_threads; i++) {
        if (pthread_create(&threads[i], NULL, woman_thread, NULL) != 0) {
            printf("Failed to create woman thread");
            running = 0;
            return THREAD_ERROR;
        }
    }
    for (int i = 0; i < men_threads; i++) {
        if (pthread_create(&threads[women_threads + i], NULL, man_thread, NULL) != 0) {
            printf("Failed to create man thread");
            running = 0;
            return THREAD_ERROR;
        }
    }

    
    for (int i = 0; i < total_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_women);
    pthread_cond_destroy(&cond_men);

    return SUCCESS;
}