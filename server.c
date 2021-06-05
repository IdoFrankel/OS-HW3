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
    // if (maxSize(waiting)!=0 && size(waiting) == maxSize(waiting)) ||
    //else (maxSize(waiting) == 0 && size(running) == maxSize(running))

    threadLockWrapper(&waiting_m);

    // BUG if maxSize(waiting) == 0 !!.
    while (size(waiting) == maxSize(waiting))
    {
        pthread_cond_wait(&waiting_insert_allow, &waiting_m);
    }

    threadUnlockWrapper(&waiting_m);
}

void OverloadHandling_DropTail(int connfd)
{
    printf("ttid = %d |\t server.c 51 - drop socket\n", gettid());
    Close(connfd);
}

void OverloadHandling_DropHead()
{
    threadLockWrapper(&waiting_m);
    int connfd = dequeue_noLock(waiting);
    threadUnlockWrapper(&waiting_m);
    Close(connfd);
}

void OverloadHandling_Random()
{
    //TODO IMPLEMENT
}

int OverloadHandling(char *schedalg, int connfd)
{
    if (strcmp(schedalg, "block") == 0)
    {
        printf("block\n");
        OverloadHandling_Block();
        return 1;
    }
    else if (strcmp(schedalg, "dt") == 0)
    {
        printf("dt\n");
        OverloadHandling_DropTail(connfd);
        return 0;
    }
    else if (strcmp(schedalg, "dh") == 0)
    {
        printf("dh\n");
        OverloadHandling_DropHead();
        return 1;
    }
    else if (strcmp(schedalg, "random") == 0)
    {
        printf("random\n");
        return 1;
    }
    else
    {
        printf("**bug**\n");
        // **bug**
        return 1;
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
        threadLockWrapper(&running_m);

        while (size(running) == 0)
        {
            // wait until the queue is not empty, and a request can be processed.
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

    getargs(&port, &maxWorkingThreads, &queue_size, &schedalg, argc, argv);

#pragma region initialize locs and conditions

    running = createQueue(min(maxWorkingThreads, queue_size));
    waiting = createQueue(queue_size - maxSize(running));

    pthread_mutex_init(&running_m, NULL);
    pthread_mutex_init(&waiting_m, NULL);

    pthread_cond_init(&running_insert_allow, NULL);
    pthread_cond_init(&running_delete_allow, NULL);

    pthread_cond_init(&waiting_insert_allow, NULL);
    pthread_cond_init(&waiting_delete_allow, NULL);

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

        // Q: not sure if locking for so long is good.
        threadLockWrapper(&running_m);
        threadLockWrapper(&waiting_m);

        printf("(1) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), size(running), queue_size);

        // should be size(running) but in dequeu o fritst dequeue, just later i run the the reques.
        //     // its a bug since i reduce the size of running, before it actuly done running.
        if (size(waiting) + size(running) == queue_size)
        {
            printf("400\n");

            // handled by part 2.
            threadUnlockWrapper(&waiting_m);
            threadUnlockWrapper(&running_m);
            if (OverloadHandling(schedalg, connfd) == 0)
            {
                // if drop tail, than we closed the previous connection and continue listening to new requests.
                continue;
            }
            threadLockWrapper(&running_m);
            threadLockWrapper(&waiting_m);
        }
        else if (size(waiting) + size(running) > queue_size)
        {
            printf("200 ****BUG *****\n");
        }

        threadUnlockWrapper(&running_m);

        // add connection to wait queue.
        enqueue_noLock(waiting, connfd);

        printf("(2) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), size(running), queue_size);

        threadUnlockWrapper(&waiting_m);

        threadLockWrapper(&running_m);

        // **** BUG ***
        // the bug is because of this while loop.
        // when ./server 8003 1 2 dt
        // first requests is queued to wating, and than dequing and to running queue.
        // seconds request is queued to waiting. but waits until there is space in running queue, which blocks future requests.
        // here  is the bug. what should we do here ???.
        while (size(running) == maxSize(running))
        {
            // wait until the queue is not empty, and a request can be processed.
            pthread_cond_wait(&running_insert_allow, &running_m);
        }

        threadLockWrapper(&waiting_m);

        printf("(3) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), size(running), queue_size);

        int temp = dequeue_noLock(waiting);         //  dequeue socket from waiting
        pthread_cond_signal(&waiting_insert_allow); //  signal waiting_insert_allow.

        // printf("(4) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), size(running), queue_size);
        enqueue_noLock(running, temp); //  enqueue to running

        printf("(5) \t waiting = %d \t running=%d \t queue_size = %d \n", size(waiting), size(running), queue_size);

        threadUnlockWrapper(&waiting_m);

        // enqueue_noLock(running, temp);              //  enqueue to running - move to line 204
        pthread_cond_signal(&running_delete_allow); //  signal running_delete_allow.
        threadUnlockWrapper(&running_m);            // unlock running queue.
    }
}
