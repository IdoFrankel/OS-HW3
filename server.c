#include "segel.h"
#include "request.h"
#include "queue.h"

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
void getargs(int *port, int *threads, int *queue_size, int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <port> <threads> <queue_size> <schedalg>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
    schedalg = argv[4];
}

#pragma region locs and conditions
int threadsCount, queueSize;
char *schedalg;

struct queue *running;
struct queue *waiting;

pthread_mutex_t running_m;
pthread_mutex_t waiting_m;

pthread_cond_t running_insert_allow;
pthread_cond_t waiting_insert_allow;

pthread_cond_t running_delete_allow;
pthread_cond_t waiting_delete_allow;

#pragma endregion

void requestHandleWrapper(int fd)
{
    printf("server.c 1\n");
    requestHandle(fd);

    printf("server.c 2\n");

    // dequeue element and send running_insert_allow signal.
    int returnedFd = dequeue(running, &running_m, &running_insert_allow);

    if (returnedFd != fd)
    {
        printf(" *** BUG  ***");
        printf("dequing from running queue returned diffrent socket to close.\n");
    }

    printf("server.c 3\n");

    // close socket fd
    Close(fd);
}

void requestWaitingHandleWrapper()
{
    //TODO- - BUG ?
    // should lock the waiting queue ?

    // waits until available work thread.
    pthread_mutex_lock(&running_m);
    while (size(running) == threadsCount)
    {
        pthread_cond_wait(&running_insert_allow, &running_m);
    }
    printf("waiting handler wrapper");

    // dequeue waiting requests, and adds it to running queue.
    pthread_mutex_lock(&waiting_m);
    int fd = dequeue_noLock(waiting);
    enqueue_noLock(running, fd);
    // sends signal to notify empty space in waiting queue.
    pthread_cond_signal(&waiting_insert_allow);

    // release both queues.
    pthread_mutex_unlock(&waiting_m);
    pthread_mutex_unlock(&running_m);

    // not sure if supose to be inside the previous lock.
    pthread_t t;
    pthread_create(&t, NULL, requestHandleWrapper, fd);
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    // int threadsCount, queueSize;

    getargs(&port, &threadsCount, &queueSize, argc, argv);

    /* 
        HW3: Create some threads...

        create fix size pool for working threads.
        each thread is blocked until there is a http request for it to handle
        if (|worker threads| > |active requests |) some worker threads are blocked, waiting for new http requests
        if (|requests| > |worker threads |) the request are buffred until there is a ready thread.

        the master thread responsible for accepting new http connections, and placing the descriptor in the queue (size is input)
        the master:
            1. accept the connection
            2. place the connection descriptor into the queue
            3. return to accepting more conntections (letting the worker thread to handle the requests).
    
        a worker thread wakes when there is an http request in the queue. (should pick the oldest e.g the first)
        worker thread:
            1. preforms read on the nerwork descriptor
            2. obtain the specified content (by either reading the static file or executing the cgi process)
            3. returns the content to the client by writing to the descriptor
            4. the worker thread then waits for another request

        NOTE: the access to the shared buffer MUST be synchronized.
            the master must hold and wait if the queue is full.
            the worker thread must wait if the queue is empty.

        HINT: seperate the queue into two:
            1. queue 1 : requests waiting to be picked up by worker thread
            2. queue 2 : requests currently handeled by some worker threads
            make sure ther combined size <= sized specified in command line

    */

    running = createQueue();
    waiting = createQueue();

    listenfd = Open_listenfd(port);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);

        //
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads
        // do the work.
        //

#pragma region initialize locs and conditions
        // pherhaps need for each queue diffrent cond_t.
        // plus cond_t for when a reuqest is finished. ??

        pthread_mutex_init(&running_m, NULL);
        pthread_mutex_init(&waiting_m, NULL);

        pthread_cond_init(&running_insert_allow, NULL);
        pthread_cond_init(&running_delete_allow, NULL);

        pthread_cond_init(&waiting_insert_allow, NULL);
        pthread_cond_init(&waiting_delete_allow, NULL);
#pragma endregion

        if (size(running) < threadsCount)
        {
            printf("server.c 20\n");
            enqueue(running, connfd, &running_m, &running_delete_allow);
            pthread_t t;
            printf("server.c 21\n");
            pthread_create(&t, NULL, requestHandleWrapper, connfd);
        }
        else
        {
            printf("server.c 30\n");
            // waiting queue is not full
            if (size(waiting) < queueSize - size(running))
            {
                printf("server.c 31\n");

                AddRequestToWaitingQueue(connfd);
                // should wait for signal from dequeuing of running (running_c)

                // 1. lock waiting queue. and dequeue element.
                // 2. lock running queue and enqueue element.
                // 3. run the newly added element.
            }
            else
            {
                printf("server.c 50\n");

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
            }
        }
    }
}

void AddRequestToWaitingQueue(connfd)
{
    enqueue(waiting, connfd, &waiting_m, &waiting_delete_allow);
    printf("server.c 32\n");

    pthread_t t;
    pthread_create(&t, NULL, requestWaitingHandleWrapper, NULL);
    printf("parent after requestWaitingHandleWrapper 4\n");
}

#pragma region overload handling procedures

void OverloadHandling_Block(int connfd)
{
    // ===== Hold and block new requests, until there is space in waiting queue.

    pthread_mutex_lock(&waiting_m);

    printf("server.c 51\n");

    while (size(waiting) + size(running) == queueSize)
    {
        // the cond_t is for dequeuing from waiting.
        pthread_cond_wait(&waiting_m, &waiting_insert_allow);
    }

    printf("server.c 52\n");

    // enqueue to waiting without locking.
    enqueue_noLock(waiting, connfd);
    // signal  pthread_cond_signal(&waiting_delete_allow); ?

    printf("server.c 53\n");

    pthread_mutex_unlock(&waiting_m);
}

void OverloadHandling_DropTail(int connfd)
{
    Close(connfd);
}

void OverloadHandling_DropHead(int connfd)
{
    // DEQUEUE FROM WAITING.

    // delete the thread itself, it is not enough just to remove from queue. ??

    // ENQUEUE TO WAITING.
    // AddRequestToWaitingQueue(connfd);
}

void OverloadHandling_Random(int connfd)
{
    //TODO IMPLEMENT
}

#pragma endregion
