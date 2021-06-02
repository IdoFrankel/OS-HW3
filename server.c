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

#pragma region locs and conditions
int maxWorkingThreads, queueSize;
char *schedalg;

struct queue *running;
struct queue *waiting;

pthread_mutex_t running_m;
pthread_mutex_t waiting_m;

pthread_cond_t running_insert_allow;
pthread_cond_t waiting_insert_allow;

pthread_cond_t running_delete_allow;
// no sure if needed.
pthread_cond_t waiting_delete_allow;

#pragma endregion

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

//deqeue **
void WorkerThreadsHandler(int fd)
{
    printf("ttid = %d |\t server.c 20\n", gettid());
    int val;
    while (1)
    {
        threadLockWrapper(&running_m);

        while (size(running) == 0)
        {
            // wait unntil the queue is not empty, and a request can be processed.
            val = pthread_cond_wait(&running_delete_allow, &running_m);
        }

        int fd = dequeue_noLock(running);
        pthread_cond_signal(&running_insert_allow);

        threadUnlockWrapper(&running_m);

        //process the request, and than close the connection.
        requestHandle(fd);
        Close(fd);
    }
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

#pragma region initialize locs and conditions

    running = createQueue();
    waiting = createQueue();

    pthread_mutex_init(&running_m, NULL);
    pthread_mutex_init(&waiting_m, NULL);

    pthread_cond_init(&running_insert_allow, NULL);
    pthread_cond_init(&running_delete_allow, NULL);

    pthread_cond_init(&waiting_insert_allow, NULL);
    pthread_cond_init(&waiting_delete_allow, NULL);

#pragma endregion

    /*
        Running quque size = maxWorkingThreads
        queueSize = running + waiting queue
        => waitingSize = queueSize - threads count
    */
    getargs(&port, &maxWorkingThreads, &queueSize, &schedalg, argc, argv);
    printf("server.c socketFD =%d running =%d \t maxWorkingThreads =%d\n", connfd, size(running), maxWorkingThreads);
    printf("ttid = %d |\t server.c 10\n", gettid());

    int val;
    // Create the worket threads poll
    for (int i = 0; i < maxWorkingThreads; i++)
    {
        pthread_t t;
        val = pthread_create(&t, NULL, WorkerThreadsHandler, connfd);
        if (val)
        {
            // error while creating thread.
        }
    }

    listenfd = Open_listenfd(port);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);

        /*
            lock both queues.
            check if  running is fully ocupied.
            if not - insert to running.
            if yes - check if waiting is fully ocuupied.
            if not - insert to waiting
            if yes - ignore, and later do part 2
        */

        // Q: should lock both, or just running now & waiting later.
        // lock both queues.
        threadLockWrapper(&running_m);
        threadLockWrapper(&waiting_m);

        if (size(running) < maxWorkingThreads)
        {
            // insert request to running queue.
            enqueue_noLock(&running, connfd);
            pthread_cond_signal(&running_delete_allow);
        }
        else if (size(waiting) < queueSize - size(running))
        {
            // insert requesut to waitng queue
            enqueue_noLock(&waiting, connfd);
            pthread_cond_signal(&waiting_delete_allow);
        }
        else
        {
            // handle the request according to part2.
        }

        // unlock both queues in reversed order.
        threadUnlockWrapper(&waiting_m);
        threadUnlockWrapper(&running_m);
    }
}

/*

 if (strcasecmp(schedalg, "block"))
            {
                OverloadHandling_Block(connfd);
            }
            else if (strcasecmp(schedalg, "dt"))
            {
                OverloadHandling_DropTail(connfd);
            }
            else if (strcasecmp(schedalg, "sh"))
            {
            }
            else if (strcasecmp(schedalg, "random"))
            {
            }



            
void OverloadHandling_DropTail(int connfd)
{
    printf("ttid = %d |\t server.c 60 - close overload socket\n", gettid());
    Close(connfd);
}

void OverloadHandling_DropHead(int connfd)
{
    //TODO IMPLEMENT
}

void OverloadHandling_Random(int connfd)
{
    //TODO IMPLEMENT
}

*/