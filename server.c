#include "segel.h"
#include "request.h"
#include "queue.h"
#include "stats.h"

#define TO_MILLi 1000
#define QUARTER(num) 0.25 * num

//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

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

#pragma region global stats
struct stats **stats_arr;
//struct timeval current_time;
#pragma endregion

#pragma region locs and conditions
int maxWorkingThreads, queue_size, runningSize, maxRunningSize;
char *schedalg;

struct queue *waiting;

pthread_mutex_t lock;
pthread_cond_t emptyWorkerThread;
pthread_cond_t UnblockingMainThread;

void threadUnlockWrapper(pthread_mutex_t *lock)
{
    if (pthread_mutex_unlock(lock) != 0)
    {
        // ERROR while unlocking.
        exit(1);
    }
}

void threadLockWrapper(pthread_mutex_t *lock)
{
    if (pthread_mutex_lock(lock) != 0)
    {
        // ERROR while locking.
        exit(1);
    }
}

#pragma endregion

#pragma region overload handling

void OverloadHandling_Block()
{
    while (size(waiting) + runningSize == queue_size)
    {
        pthread_cond_wait(&UnblockingMainThread, &lock);
    }
}

void OverloadHandling_DropTail(int conn)
{
    threadUnlockWrapper(&lock);
    Close(conn);
    threadLockWrapper(&lock);
}

void OverloadHandling_DropHead()
{
    int tempConn = dequeue_noLock(waiting, NULL);
    threadUnlockWrapper(&lock);
    Close(tempConn);
    threadLockWrapper(&lock);
}

void OverloadHandling_Random()
{
    int drop_count = my_ceil(QUARTER(size(waiting)));
    while (drop_count != 0)
    {
        // random numbers between 1 to drop_count
        int drop = (rand() % drop_count) + 1;
        int drop_fd = dequeueByOrder(waiting, drop);
        close(drop_fd);
        drop_count--;
    }
    return;
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
        OverloadHandling_Random();
    }
    else
    {
        // if you have reached here, there is a bug.
        exit(1);
    }
}

#pragma endregion

//deqeue **
void WorkerThreadsHandler(void *stats_i)
{
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
            // If reached here, there is a bug.
        }

        struct timeval *current_time = malloc(sizeof(*current_time));
        //unsigned long int time_in_ms;
        struct timeval *arrivel_time = malloc(sizeof(*arrivel_time));
        connfd = dequeue_noLock(waiting, arrivel_time);
        gettimeofday(current_time, NULL);
        setArrivelTime((struct stats *)stats_i, arrivel_time);
        //time_in_ms = current_time.tv_sec * TO_MILLi + current_time.tv_usec / TO_MILLi- time_in_ms;
        setDispatchTime((struct stats *)stats_i, current_time);
        runningSize += 1;

        threadUnlockWrapper(&lock);

        //process the request, and than close the connection.
        requestHandle(connfd, (struct stats *)stats_i);

        Close(connfd);

        threadLockWrapper(&lock);
        runningSize -= 1;
        pthread_cond_signal(&UnblockingMainThread);
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
    pthread_cond_init(&UnblockingMainThread, NULL);

#pragma endregion

#pragma region Worker thread poll initializing
    int val;
    stats_arr = malloc(sizeof(*stats_arr) * maxWorkingThreads);
    for (int i = 0; i < maxWorkingThreads; i++)
    {
        pthread_t t;
        stats_arr[i] = createstats(i);
        val = pthread_create(&t, NULL, WorkerThreadsHandler, stats_arr[i]);
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
        struct timeval *current_time = malloc(sizeof(*current_time));
        //unsigned long int time_in_ms;
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);
        gettimeofday(current_time, NULL);

        threadLockWrapper(&lock);

        if (size(waiting) + runningSize == queue_size)
        {
            // if drop-head, but the waiting queue is empty, should ignore the new request.
            if (size(waiting) == 0 && ((strcmp(schedalg, "dh") == 0) || (strcmp(schedalg, "random") == 0)))
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

        // add request to waiting queue.
        enqueue_noLock(waiting, connfd, current_time);

        // should signal in main to only maxRunningSize threads,
        // any additional thread should be waked up when other requests is done.
        if (runningSize < maxRunningSize)
        {
            // Signal any unemployed worker-thread can wake-up.
            pthread_cond_signal(&emptyWorkerThread);
        }

        threadUnlockWrapper(&lock);
    }
}
