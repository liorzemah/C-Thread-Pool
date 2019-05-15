#include "threadpool.h"

/*
 * Method: tpCreate
 * Task:   Create pool thread
 * @param intOfThread number of threads
 */
ThreadPool *tpCreate(int intOfThread)
{
    ThreadPool *pool;
    int i;

    if(intOfThread <= 0 || intOfThread > MAX_THREADS || QUEUE_SIZE <= 0 || QUEUE_SIZE > MAX_QUEUE)
    {
        fprintf(stderr, "Queue/Thread overflow\n");
        return NULL;
    }

    if((pool = (ThreadPool *)malloc(sizeof(ThreadPool))) == NULL)
    {
        fprintf(stderr, "Dynamic allocation failed\n");
        return NULL;
    }

    //init
    pool->thread_count = 0;
    pool->queue_size = QUEUE_SIZE;
    pool->count = 0;
    pool->shutdown = -1;
    pool->started = 0;

    /* Allocate thread and Task queue */
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * intOfThread);
    pool->queue = osCreateQueue();

    /* Initialize mutex and conditional variable first */
    if((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
       (pthread_cond_init(&(pool->notify), NULL) != 0) ||
       (pool->threads == NULL) ||
       (pool->queue == NULL))
    {
        tpFree(pool);
        return NULL;
    }

    /* Start worker threads */
    for(i = 0; i < intOfThread; i++)
    {
        if(pthread_create(&(pool->threads[i]), NULL,
                          tpThread, (void*)pool) != 0)
        {
            tpDestroy(pool, 0);
            return NULL;
        }
        pool->thread_count++;
        pool->started++;
    }

    return pool;
}

/*
 * Method: tpInsertTask
 * Task: Insert task to queue
 * @param ThreadPool *threadPool - pool of threads
 * @param void (*computeFunc) (void *) - task function pointer
 * @param void* param - arguments to task function pointer
 * @ret 0 - good , -1 - bad
 */
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param)
{
    int err = 0;

    if(threadPool == NULL || computeFunc == NULL) {
        fprintf(stderr, "Thread Pool is invalid\n");
        return -1;
    }

    if (threadPool->started == 0)
    {
        fprintf(stderr, "ThreadPool unstarted\n");
        return -1;
    }

    if(pthread_mutex_lock(&(threadPool->lock)) != 0) {
        fprintf(stderr, "ThreadPool lock failure\n");
        return -1;
    }

    /* Queue is full */
    if (threadPool->count == threadPool->queue_size) {
        fprintf(stderr, "ThreadPool queue is full\n");
    }

    /* Are we shutting down ? */
    if (threadPool->shutdown != -1) {
        fprintf(stderr, "ThreadPool already shutdown\n");
        err = -1;
    }

    /* Add Task to queue */
    if (err == 0) {
        Task *task = (Task *) malloc(sizeof(Task));
        task->method = computeFunc;
        task->param = param;
        osEnqueue(threadPool->queue, task);
        threadPool->count += 1;
    }


    /* pthread_cond_broadcast */
    if (pthread_cond_signal(&(threadPool->notify)) != 0) {
        fprintf(stderr, "ThreadPool lock failure\n");
        err =-1;
    }


    if(pthread_mutex_unlock(&threadPool->lock) != 0) {
        fprintf(stderr, "ThreadPool lock failure\n");
        err = -1;
    }

    return err;
}

/*
 * Method: tpDestroy
 * Task: Destroy thread pool
 * @param ThreadPool *threadPool - pool of threads
 * @param shutdownType - 0 - immediate shutdown, 1 - graceful shutdown
 */
void tpDestroy(ThreadPool* threadPool, int shutdownType)
{
    int i, err = 0;

    if(threadPool == NULL) {
        fprintf(stderr, "ThreadPool is invalid\n");
        return;
    }

    if(pthread_mutex_lock(&(threadPool->lock)) != 0) {
        fprintf(stderr, "ThreadPool lock failure\n");
        return;
    }

    /* Already shutting down */
    if (threadPool->shutdown != -1) {
        fprintf(stderr, "ThreadPool already shutdown\n");
    }

    threadPool->shutdown = shutdownType;


    /* Wake up all worker threads */
    if ((pthread_cond_broadcast(&(threadPool->notify)) != 0) ||
        (pthread_mutex_unlock(&(threadPool->lock)) != 0)) {
        err = -1;
        fprintf(stderr, "ThreadPool lock failure\n");
    }
    /* Join all worker thread */
    for (i = 0; i < threadPool->thread_count; i++) {
        if (pthread_join(threadPool->threads[i], NULL) != 0) {
            err = -1;
            fprintf(stderr, "ThreadPool lock failure\n");
        }
    }

    /* Only if everything went well do we deallocate the pool */
    if(!err) {
        tpFree(threadPool);
    }
}
/*
 * Method: tpFree
 * Task: deallocate thread pool
 * @param pool - pool of threads
 */
int tpFree(ThreadPool *pool)
{
    if(pool == NULL || pool->started > 0) {
        return -1;
    }

    if(pool->threads) {
        free(pool->threads);
        pool->threads = NULL;

        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
    }

    if(pool->queue) {
        osDestroyQueue(pool->queue);
        pool->queue = NULL;
    }
    free(pool);
    return 0;
}

/*
 * Method: tpThread
 * Task: Build thread and execute tasks
 * @param threadpool - pool of threads
 */
static void *tpThread(void *threadpool)
{
    ThreadPool *pool = (ThreadPool *)threadpool;
    Task* task;

    for(;;)
    {
        pthread_mutex_lock(&(pool->lock));

        /* Wait on condition variable, check for spurious wakeups.
           When returning from pthread_cond_wait(), we own the lock. */
        while((pool->count == 0) && (pool->shutdown == -1))
        {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if((pool->shutdown == IMMEDIATE_SHUTDOWN) ||
           ((pool->shutdown == GRACEFUL_SHUTDOWN) &&
            (pool->count == 0)))
        {
            break;
        }

        //Get task
        task = (Task*)osDequeue(pool->queue);
        pool->count -= 1;

        //Unlock mutex
        pthread_mutex_unlock(&(pool->lock));

        //Execute task
        (*(task->method))(task->param);


        free(task);
    }

    pool->started--;

    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
}