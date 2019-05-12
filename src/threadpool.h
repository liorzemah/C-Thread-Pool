#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "osqueue.h"

#define MAX_THREADS 64
#define MAX_QUEUE 65536
#define THREADS_SIZE 10
#define QUEUE_SIZE   80

typedef struct thread_pool
{
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    OSQueue* queue;
    int thread_count;
    int queue_size;
    int count;
    int shutdown;
    int started;
}ThreadPool;

typedef enum
{
    IMMEDIATE_SHUTDOWN = 0,
    GRACEFUL_SHUTDOWN  = 1,
} ShutdownTypes;

typedef struct
{
    void (*method)(void *);
    void *param;
} Task;

ThreadPool *tpCreate(int intOfThread);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

void tpDestroy(ThreadPool* threadPool, int shutdownType);

int tpFree(ThreadPool *pool);

static void *tpThread(void *threadpool);

#endif