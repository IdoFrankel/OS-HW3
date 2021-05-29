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
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    int threadsCount, queueSize;

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

    struct queue *running = createQueue();
    struct queue *waiting = createQueue();

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

        pthread_cond_t running_c;
        pthread_mutex_t running_m;
        pthread_cond_init(&running_c, NULL);
        pthread_mutex_init(&running_m, NULL);

        pthread_cond_t waiting_c;
        pthread_mutex_t waiting_m;
        pthread_cond_init(&waiting_c, NULL);
        pthread_mutex_init(&waiting_m, NULL);
#pragma endregion
        if (size(running) < threadsCount)
        {
            enqueue(running, connfd, &running_m, &running_c);
            pthread_t t;
            pthread_create(&t, NULL, requestHandle, connfd);
            printf("inside 4\n");

            printf("print after pthread_create line 107 server\n");

            // how do we dequeue from ruuning and sending signal ? create a wrapper method to requestsHandler ?
            // dequeue(running, &running_m, &running_c);
            // send close(connfd) as well.
        }
        else
        {
            // waiting queue is not full
            if (size(waiting) < queueSize - size(running))
            {
                enqueue(waiting, connfd, &waiting_m, &waiting_c);

                // should wait for signal from dequeuing of running (running_c)

                // 1. lock waiting queue. and dequeue element.
                // 2. lock running queue and enqueue element.
                // 3. run the newly added element.

                /* -- not completed code --
                    //not entierly sure.
                    pthread_mutex_lock(&m);
                    while (size(running) == threadsCount)
                    {
                        pthread_cond_wait(&c, &m);
                    }
                    insert first item in waiting to last in qnqueue and create thread for it.
                    pthread_mutex_unlock();
                    */
            }
            else
            {
                // ===== Hold and block new requests, until there is space in waiting queue.

                // todo - should  lock running queue as well ?  because using size(running)
                pthread_mutex_lock(&waiting_m);

                while (size(waiting) + size(running) == queueSize)
                {
                    // the cond_t is for dequeuing from waiting.
                    pthread_cond_wait(&waiting_c, &waiting_m);
                }

                // enqueue to waiting without locking.
                enqueue_noLock(waiting, connfd);
                pthread_mutex_unlock(&waiting_m);
            }
        }
    }
}
