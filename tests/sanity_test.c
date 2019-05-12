#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../src/osqueue.h"
#include "../src/threadpool.h"

void hello (void* a)
{
    printf("hello: %d\n", (*((int*)a)));
}

int main()
{
    int i;

    ThreadPool* tp = tpCreate(5);

    for(i=0; i<5; ++i)
    {
        tpInsertTask(tp,hello,&i);
        usleep(1000);
    }

    tpDestroy(tp, 1);
    return 0;
}