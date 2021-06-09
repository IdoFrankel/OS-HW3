#include <stdlib.h>
#include <pthread.h>
#include "segel.h"

// TODO remove this when submiting.
#include <unistd.h>
#include <sys/syscall.h>

#ifndef SYS_gettid
#error "SYS_gettid unavailable on this system"
#endif

#define gettid() ((pid_t)syscall(SYS_gettid))

// TODO remove above when submitting

typedef struct node
{
    int data;
    unsigned long request_arrivel;
    struct node *next, *prev;
} node;

typedef struct queue
{
    int size, maxSize;
    node *front, *back;
} queue;

struct queue *createQueue(int maxSize)
{
    struct queue *q = malloc(sizeof(struct queue));
    q->size = 0;
    q->maxSize = maxSize;
    q->back = q->front = NULL;
    return q;
}

void enqueue_noLock(struct queue *q, int data, unsigned long req_arrivel)
{
    //printf("ttid = %d |\t queue.c 3\n", gettid());
    node *n = (struct node *)malloc(sizeof(struct node));
    n->next = NULL;
    n->prev = NULL;
    n->data = data;
    n->request_arrivel = req_arrivel;

    //printf("ttid = %d |\t queue.c 4\n", gettid());
    if (q->front == NULL)
    {
        q->front = n;
    }

    if (q->back != NULL)
    {
        n->prev = q->back;
        q->back->next = n;
    }

    //printf("ttid = %d |\t queue.c 5\n", gettid());
    q->back = n;
    q->size += 1;
    //printf("ttid = %d |\t queue.c 6\n", gettid());
}

int dequeue_noLock(struct queue *q, unsigned long* arrivel)
{
    //printf("ttid = %d |\t queue.c 12\n", gettid());

    node *old_head = q->front;
    //printf("ttid = %d |\t queue.c 12.1\n", gettid());

    q->front = q->front->next;
    if (q->front != NULL)
    {
        q->front->prev = NULL;
    }
    else
    {
        //meaning the old_head was both front and back node of the queue.
        q->back = NULL;
    }

    //printf("ttid = %d |\t queue.c 12.2\n", gettid());
    q->size -= 1;
    //printf("ttid = %d |\t queue.c 12.3\n", gettid());

    old_head->next = NULL;
    int data = old_head->data;
    if(arrivel != NULL){
        *arrivel = old_head->request_arrivel;
    }
    
    //printf("ttid = %d |\t queue.c 12.4\n", gettid());

    free(old_head);

    //printf("ttid = %d |\t queue.c 12.5\n", gettid());

    return data;
}

int dequeueByFd(struct queue *q, int fd)
{
    // printf("ttid = %d |\t queue.c 17\n", gettid());

    struct node *n = q->front;
    while (n->next != NULL && n->data != fd)
    {
        n = n->next;
    }

    if (n->next == NULL && n->data != fd)
    {
        // IF you have reached here, there is a *** BUG ***
        char content[MAXBUF];
        //sprintf(content, "%s %d wasnt inside the queue\n", content, fd);
        app_error(content);
        return -1;
    }
    else
    {
        // remove n
        if (q->front == n)
        {
            q->front = n->next;
        }
        if (q->back == n)
        {
            q->back = n->prev;
        }

        if (n->next != NULL)
        {
            n->next->prev = n->prev;
        }
        if (n->prev != NULL)
        {
            n->prev->next = n->next;
        }

        q->size -= 1;
        int returnedFd = n->data;
        free(n);
        return returnedFd;
    }
}

int size(struct queue *q)
{
    int size = q->size;
    return size;
}

int maxSize(struct queue *q)
{
    return q->maxSize;
}
