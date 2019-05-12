#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include "../src/threadpool.h"

#define USLEEP_TIME 10000

int tasksInQueue = 0, tasksDone = 0;
pthread_mutex_t lock;

void sleepTask(void *arg) {
    usleep(*((int*)arg));
    pthread_mutex_lock(&lock);
    tasksDone++;
    pthread_mutex_unlock(&lock);
}

int main(int argc, char **argv)
{
    ThreadPool *pool;

    pthread_mutex_init(&lock, NULL);

    int time = USLEEP_TIME;
    void* arg = (void*)&time;

    /*******************IMMEDIATE_SHUTDOWN test**********************/

    assert((pool = tpCreate(THREADS_SIZE)) != NULL);
    fprintf(stderr, "Pool started with %d threads and "
                    "queue size of %d\n", THREADS_SIZE, QUEUE_SIZE);

    for(int i=0;i<1000 && tpInsertTask(pool, &sleepTask, arg) == 0 ;i++) {
        pthread_mutex_lock(&lock);
        tasksInQueue++;
        pthread_mutex_unlock(&lock);
        usleep(100);
    }

    printf("Inserted %d tasks\n", tasksInQueue);

    tpDestroy(pool, IMMEDIATE_SHUTDOWN);

    printf("Did %d tasks\n", tasksDone);
    /****************************************************************/

    tasksInQueue = 0;
    tasksDone = 0;
    /*******************GRACEFUL_SHUTDOWN test**********************/

    assert((pool = tpCreate(THREADS_SIZE)) != NULL);
    fprintf(stderr, "Pool started with %d threads and "
                    "queue size of %d\n", THREADS_SIZE, QUEUE_SIZE);


    for(int i=0;i<1000 && tpInsertTask(pool, &sleepTask, arg) == 0 ;i++) {
        pthread_mutex_lock(&lock);
        tasksInQueue++;
        pthread_mutex_unlock(&lock);
        usleep(100);
    }

    printf("Inserted %d tasks\n", tasksInQueue);

    tpDestroy(pool, GRACEFUL_SHUTDOWN);

    printf("Did %d tasks\n", tasksDone);

    /****************************************************************/

    return 0;
}
