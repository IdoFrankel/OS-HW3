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
int maxWorkingThreads, queue_size;
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
void WorkerThreadsHandler()
{
    // printf("ttid = %d |\t server.c 20\n", gettid());
    int connfd;
    while (1)
    {
        threadLockWrapper(&running_m);

        while (size(running) == 0)
        {
            // wait unntil the queue is not empty, and a request can be processed.
            pthread_cond_wait(&running_delete_allow, &running_m);
        }

        connfd = dequeue_noLock(running);
        increaseSize(running);
        threadUnlockWrapper(&running_m);
        //process the request, and than close the connection.
        requestHandle(connfd);
        Close(connfd);

        threadLockWrapper(&running_m);
        decreaseSize(running);
        pthread_cond_signal(&running_insert_allow);
        threadUnlockWrapper(&running_m);
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
        queue_size = running + waiting queue
        => waitingSize = queue_size - threads count
    */

    getargs(&port, &maxWorkingThreads, &queue_size, &schedalg, argc, argv);
    // printf("ttid = %d |\t server.c 10\n", gettid());

    int val;
    // Create the worket threads poll
    for (int i = 0; i < maxWorkingThreads; i++)
    {
        pthread_t t;
        val = pthread_create(&t, NULL, WorkerThreadsHandler, i);
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

        // Q: not sure if locking for so long is good.
        threadLockWrapper(&waiting_m);
        threadLockWrapper(&running_m);

        // BUG, SHOULD HAPPEND IF Queue_size <= threads ?
        // what is the running queue size, and waiting queue size.

        printf("(1) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), size(running), queue_size);

        // should be size(running) but in dequeu o fritst dequeue, just later i run the the reques.
        //     // its a bug since i reduce the size of running, before it actuly done running.
        if (size(waiting) + size(running) >= queue_size)
        {
            // handle by part 2.
            // handle by section 2.
            printf("(55) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), size(running), queue_size);

            threadUnlockWrapper(&running_m); //tmp
            threadUnlockWrapper(&waiting_m); //temp
            Close(connfd);
            continue;
        }

        threadUnlockWrapper(&running_m);

        // add connection to wait queue.
        enqueue_noLock(waiting, connfd);

        printf("(2) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), size(running), queue_size);

        threadUnlockWrapper(&waiting_m); // not sure

        // not quite sure about the condition.
        threadLockWrapper(&running_m);
        while (size(running) == maxWorkingThreads)
        {
            // wait unntil the queue is not empty, and a request can be processed.
            pthread_cond_wait(&running_insert_allow, &running_m);
        }

        threadLockWrapper(&waiting_m);

        printf("(3) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), size(running), queue_size);

        int temp = dequeue_noLock(waiting); //  dequeue socket from waiting

        printf("(4) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), size(running), queue_size);

        threadUnlockWrapper(&waiting_m);

        enqueue_noLock(running, temp);              //  enqueue to running
        pthread_cond_signal(&running_delete_allow); //  signal running_delete_allow.
        threadUnlockWrapper(&running_m);            // unlock running queue.
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