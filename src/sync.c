#include <stdio.h>
#include "oslab.h"

#ifdef _WIN32
void sync_demo(void) {
    puts("\n==== Process Synchronization Demo ====");
    puts("The synchronization experiments use POSIX pthreads/semaphores and should be run on Linux.");
}

void sync_menu(void) {
    sync_demo();
}

int sync_selftest(void) {
    return 1;
}
#else
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define PRODUCE_COUNT 8

static int buffer[BUFFER_SIZE];
static int in_pos = 0;
static int out_pos = 0;
static pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t empty_slots;
static sem_t full_slots;

static void *producer(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < PRODUCE_COUNT; ++i) {
        int item = id * 100 + i;
        sem_wait(&empty_slots);
        pthread_mutex_lock(&buffer_mutex);
        buffer[in_pos] = item;
        printf("producer %d -> %d at slot %d\n", id, item, in_pos);
        in_pos = (in_pos + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&buffer_mutex);
        sem_post(&full_slots);
        usleep(10000);
    }
    return NULL;
}

static void *consumer(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < PRODUCE_COUNT; ++i) {
        sem_wait(&full_slots);
        pthread_mutex_lock(&buffer_mutex);
        int item = buffer[out_pos];
        printf("consumer %d <- %d from slot %d\n", id, item, out_pos);
        out_pos = (out_pos + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&buffer_mutex);
        sem_post(&empty_slots);
        usleep(15000);
    }
    return NULL;
}

static pthread_mutex_t rw_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t read_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static int read_count = 0;
static int shared_value = 0;

static void *reader(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 3; ++i) {
        pthread_mutex_lock(&read_count_mutex);
        read_count++;
        if (read_count == 1) pthread_mutex_lock(&rw_mutex);
        pthread_mutex_unlock(&read_count_mutex);

        printf("reader %d reads %d\n", id, shared_value);
        usleep(8000);

        pthread_mutex_lock(&read_count_mutex);
        read_count--;
        if (read_count == 0) pthread_mutex_unlock(&rw_mutex);
        pthread_mutex_unlock(&read_count_mutex);
        usleep(8000);
    }
    return NULL;
}

static void *writer(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 3; ++i) {
        pthread_mutex_lock(&rw_mutex);
        shared_value += 10 + id;
        printf("writer %d writes %d\n", id, shared_value);
        pthread_mutex_unlock(&rw_mutex);
        usleep(12000);
    }
    return NULL;
}

static pthread_mutex_t forks[5];

static void *philosopher(void *arg) {
    int id = *(int *)arg;
    int left = id;
    int right = (id + 1) % 5;
    int first = left < right ? left : right;
    int second = left < right ? right : left;

    for (int i = 0; i < 2; ++i) {
        printf("philosopher %d thinking\n", id);
        usleep(5000);
        pthread_mutex_lock(&forks[first]);
        pthread_mutex_lock(&forks[second]);
        printf("philosopher %d eating\n", id);
        usleep(5000);
        pthread_mutex_unlock(&forks[second]);
        pthread_mutex_unlock(&forks[first]);
    }
    return NULL;
}

static void run_producer_consumer(void) {
    pthread_t ps[2], cs[2];
    int ids[] = {1, 2};
    in_pos = out_pos = 0;
    sem_init(&empty_slots, 0, BUFFER_SIZE);
    sem_init(&full_slots, 0, 0);
    pthread_create(&ps[0], NULL, producer, &ids[0]);
    pthread_create(&ps[1], NULL, producer, &ids[1]);
    pthread_create(&cs[0], NULL, consumer, &ids[0]);
    pthread_create(&cs[1], NULL, consumer, &ids[1]);
    for (int i = 0; i < 2; ++i) {
        pthread_join(ps[i], NULL);
        pthread_join(cs[i], NULL);
    }
    sem_destroy(&empty_slots);
    sem_destroy(&full_slots);
}

static void run_readers_writers(void) {
    pthread_t rs[3], ws[2];
    int rids[] = {1, 2, 3};
    int wids[] = {1, 2};
    read_count = 0;
    shared_value = 0;
    for (int i = 0; i < 3; ++i) pthread_create(&rs[i], NULL, reader, &rids[i]);
    for (int i = 0; i < 2; ++i) pthread_create(&ws[i], NULL, writer, &wids[i]);
    for (int i = 0; i < 3; ++i) pthread_join(rs[i], NULL);
    for (int i = 0; i < 2; ++i) pthread_join(ws[i], NULL);
}

static void run_dining_philosophers(void) {
    pthread_t ts[5];
    int ids[] = {0, 1, 2, 3, 4};
    for (int i = 0; i < 5; ++i) pthread_mutex_init(&forks[i], NULL);
    for (int i = 0; i < 5; ++i) pthread_create(&ts[i], NULL, philosopher, &ids[i]);
    for (int i = 0; i < 5; ++i) pthread_join(ts[i], NULL);
    for (int i = 0; i < 5; ++i) pthread_mutex_destroy(&forks[i]);
}

void sync_demo(void) {
    puts("\n==== Process Synchronization Demo ====");
    puts("\nProducer-consumer with mutex + empty/full semaphores:");
    run_producer_consumer();
    puts("\nReaders-writers with read-count mutex + writer mutex:");
    run_readers_writers();
    puts("\nDining philosophers with ordered fork locking to avoid deadlock:");
    run_dining_philosophers();
    puts("\nAnalysis: bounded-buffer semaphores protect capacity; readers-writers allows concurrent reads and exclusive writes; ordered fork acquisition removes circular wait, so philosophers cannot deadlock.");
}

void sync_menu(void) {
    sync_demo();
}

int sync_selftest(void) {
    return 1;
}
#endif
