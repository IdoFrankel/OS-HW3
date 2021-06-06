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
        //fprintf(stderr, "Usage: %s <port> <threads> <queue_size> <schedalg>\n", argv[0]);
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
    // TODO - QUESTION
    // check if the implementation is good.
    // when blocking, if new requests came, should we ignore them, or
    // process them at later stage ??
    while (size(waiting) + runningSize == queue_size)
    {
        pthread_cond_wait(&emptyWorkerThread, &lock);
    }
}

void OverloadHandling_DropTail(int conn)
{
    //printf("ttid = %d |\t server.c 51 - drop socket\n", gettid());
    threadUnlockWrapper(&lock);
    Close(conn);
    threadLockWrapper(&lock);
}

void OverloadHandling_DropHead()
{
    int tempConn = dequeue_noLock(waiting);
    threadUnlockWrapper(&lock);
    Close(tempConn);
    threadLockWrapper(&lock);
}

void OverloadHandling_Random()
{
    //TODO IMPLEMENT
}

void OverloadHandling(char *schedalg, int conn)
{
    if (strcmp(schedalg, "block") == 0)
    {
        OverloadHandling_Block();
    }
    else if (strcmp(schedalg, "dt") == 0)
    {
        OverloadHandling_DropTail(conn);
    }
    else if (strcmp(schedalg, "dh") == 0)
    {
        OverloadHandling_DropHead();
    }
    else if (strcmp(schedalg, "random") == 0)
    {
        // TODO IMPLEMENT.
    }
    else
    {
        // if you have reached here, there is a bug.
        exit(1);
    }
}

#pragma endregion

//deqeue **
void WorkerThreadsHandler()
{
    // //printf("ttid = %d |\t server.c 20\n", gettid());
    int connfd;
    while (1)
    {
        threadLockWrapper(&lock);
        while (size(waiting) == 0 || runningSize == maxRunningSize)
        {
            // wait until the queue is not empty, and a request can be processed.
            pthread_cond_wait(&emptyWorkerThread, &lock);
        }

        if (size(waiting) == 0)
        {
            // If reached here,
            //printf("size(waiting) == 0 ** BUG **\n");
        }

        connfd = dequeue_noLock(waiting);
        runningSize += 1;

        threadUnlockWrapper(&lock);

        //printf("process connfd=%d\n", connfd);

        //process the request, and than close the connection.
        requestHandle(connfd);

        //printf("close connfd=%d\n", connfd);

        Close(connfd);

        //printf("done closing connfd=%d\n", connfd);

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
            exit(1);
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

        //printf("(1) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), runningSize, queue_size);

        if (size(waiting) + runningSize == queue_size)
        {
            // if drop-head, but the waiting queue is empty, should ignore the new request.
            if (size(waiting) == 0 && (strcmp(schedalg, "dh") == 0))
            {
                threadUnlockWrapper(&lock);
                Close(connfd);
                continue;
            }

            // handled by part 2.
            OverloadHandling(schedalg, connfd);
            if (strcmp(schedalg, "dt") == 0)
            {
                // if drop tail, than we closed the previous connection and continue listening to new requests.
                threadUnlockWrapper(&lock);
                continue;
            }
        }
        else if (size(waiting) + runningSize > queue_size)
        {
            //printf("200 ****BUG *****\n");
        }

        //printf("add connection %d to waiting queue \n", connfd);

        // add request to waiting queue.
        enqueue_noLock(waiting, connfd);

        // Signal any unemployed worker-thread can wake-up.
        pthread_cond_signal(&emptyWorkerThread);

        //printf("(2) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), runningSize, queue_size);

        threadUnlockWrapper(&lock);
    }
}
