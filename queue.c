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

void enqueue_noLock(struct queue *q, int data)
{
    //printf("ttid = %d |\t queue.c 3\n", gettid());
    node *n = (struct node *)malloc(sizeof(struct node));
    n->next = NULL;
    n->prev = NULL;
    n->data = data;

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

int dequeue_noLock(struct queue *q)
{
    //printf("ttid = %d |\t queue.c 12\n", gettid());

    node *old_head = q->front;
    //printf("ttid = %d |\t queue.c 12.1\n", gettid());

    q->front = q->front->next;
    if (q->front != NULL)
    {
        q->front->prev = NULL;
    }

    //printf("ttid = %d |\t queue.c 12.2\n", gettid());
    q->size -= 1;
    //printf("ttid = %d |\t queue.c 12.3\n", gettid());

    int data = old_head->data;
    //printf("ttid = %d |\t queue.c 12.4\n", gettid());

    free(old_head);

    //printf("ttid = %d |\t queue.c 12.5\n", gettid());

    return data;
}

// TODO shpould be locked?
// IF YES, THERE IS A PROBLEM, SINCE THERE ARE CALSL TO SIZE WITHIN LOCKS
// IN THAT CASE, RE LOCK WILL CAUSE ERROR, PROBLEM !!
int size(struct queue *q)
{
    int size = q->size;
    return size;
}

int maxSize(struct queue *q)
{
    return q->maxSize;
}