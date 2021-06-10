#include <stdlib.h>
#include <pthread.h>
#include "segel.h"

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
    node *n = (struct node *)malloc(sizeof(struct node));
    n->next = NULL;
    n->prev = NULL;
    n->data = data;

    if (q->front == NULL)
    {
        q->front = n;
    }

    if (q->back != NULL)
    {
        n->prev = q->back;
        q->back->next = n;
    }

    q->back = n;
    q->size += 1;
}

int dequeue_noLock(struct queue *q)
{

    node *old_head = q->front;

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

    q->size -= 1;

    old_head->next = NULL;
    int data = old_head->data;

    free(old_head);

    return data;
}

int dequeueByFd(struct queue *q, int fd)
{
    struct node *n = q->front;
    while (n->next != NULL && n->data != fd)
    {
        n = n->next;
    }

    if (n->next == NULL && n->data != fd)
    {
        // IF you have reached here, there is a *** BUG ***
        char content[MAXBUF];
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
