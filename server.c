#include "segel.h"
#include "request.h"
#include "queue.h"

#include <unistd.h>
#include <sys/syscall.h>

// TODO remove this when submiting.
#include <unistd.h>
#include <sys/syscall.h>

#ifndef SYS_gettid
#error "SYS_gettid unavailable on this system"
#endif

#define gettid() ((pid_t)syscall(SYS_gettid))

// TODO remove above when submitting

//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

//TODO When using pthreads library add -pthread flag to make

// HW3: Parse the new arguments too
void getargs(int *port, int *threads, int *queue_size, char **schedalg, int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <port> <threads> <queue_size> <schedalg>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
    *schedalg = argv[4];
}

int min(int a, int b)
{
    return (a < b) ? a : b;
}

#pragma region locs and conditions
int maxWorkingThreads, queue_size, runningSize, maxRunningSize;
char *schedalg;

struct queue *waiting;

pthread_mutex_t lock;
pthread_cond_t emptyWorkerThread;

void threadUnlockWrapper(pthread_mutex_t *lock)
{
    if (pthread_mutex_unlock(lock) != 0)
    {
        // ERROR while unlocking.
    }
}

void threadLockWrapper(pthread_mutex_t *lock)
{
    if (pthread_mutex_lock(lock) != 0)
    {
        // ERROR while locking.
    }
}

#pragma endregion

#pragma region overload handling

void OverloadHandling_Block()
{
    threadLockWrapper(&lock);

    while (size(waiting) == maxSize(waiting))
    {
        pthread_cond_wait(&emptyWorkerThread, &lock);
    }

    threadUnlockWrapper(&lock);
}

void OverloadHandling_DropTail(int connfd)
{
    printf("ttid = %d |\t server.c 51 - drop socket\n", gettid());
    Close(connfd);
}

void OverloadHandling_DropHead()
{
    threadLockWrapper(&lock);
    int connfd = dequeue_noLock(waiting);
    threadUnlockWrapper(&lock);
    Close(connfd);
}

void OverloadHandling_Random()
{
    //TODO IMPLEMENT
}

void OverloadHandling(char *schedalg, int connfd)
{
    if (strcmp(schedalg, "block") == 0)
    {
        printf("block\n");
        OverloadHandling_Block();
    }
    else if (strcmp(schedalg, "dt") == 0)
    {
        printf("dt\n");
        OverloadHandling_DropTail(connfd);
    }
    else if (strcmp(schedalg, "dh") == 0)
    {
        printf("dh\n");
        OverloadHandling_DropHead();
    }
    else if (strcmp(schedalg, "random") == 0)
    {
        printf("random\n");
    }
    else
    {
        printf("**bug**\n");
        // **bug**
    }
}

#pragma endregion

//deqeue **
void WorkerThreadsHandler()
{
    // printf("ttid = %d |\t server.c 20\n", gettid());
    int connfd;
    while (1)
    {
        threadLockWrapper(&lock);
        while (size(waiting) == 0 || runningSize == maxRunningSize)
        {
            // wait until the queue is not empty, and a request can be processed.
            pthread_cond_wait(&emptyWorkerThread, &lock);
        }

        connfd = dequeue_noLock(waiting);
        runningSize += 1;

        threadUnlockWrapper(&lock);

        //process the request, and than close the connection.
        requestHandle(connfd);
        Close(connfd);

        threadLockWrapper(&lock);
        runningSize -= 1;
        pthread_cond_signal(&emptyWorkerThread);
        threadUnlockWrapper(&lock);
    }
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    getargs(&port, &maxWorkingThreads, &queue_size, &schedalg, argc, argv);

#pragma region initialize locs and conditions

    runningSize = 0;
    maxRunningSize = min(maxWorkingThreads, queue_size);
    waiting = createQueue(queue_size - maxRunningSize);

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&emptyWorkerThread, NULL);

#pragma endregion

#pragma region Worker thread poll initializing
    int val;
    for (int i = 0; i < maxWorkingThreads; i++)
    {
        pthread_t t;
        val = pthread_create(&t, NULL, WorkerThreadsHandler, i);
        if (val)
        {
            // error while creating thread.
        }
    }
#pragma endregion

    listenfd = Open_listenfd(port);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);

        threadLockWrapper(&lock);

        printf("(1) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), runningSize, queue_size);

        if (size(waiting) + runningSize == queue_size)
        {
            // handled by part 2.
            threadUnlockWrapper(&lock);
            OverloadHandling(schedalg, connfd);
            if (strcmp(schedalg, "dt") == 0)
            {
                // if drop tail, than we closed the previous connection and continue listening to new requests.
                continue;
            }
            threadLockWrapper(&lock);
        }
        else if (size(waiting) + runningSize > queue_size)
        {
            printf("200 ****BUG *****\n");
        }

        // add request to waiting queue.
        enqueue_noLock(waiting, connfd);

        // Signal that if there is an empty worker-thread, than there is a waiting request for it.
        pthread_cond_signal(&emptyWorkerThread);

        printf("(2) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), runningSize, queue_size);

        threadUnlockWrapper(&lock);
    }
}
