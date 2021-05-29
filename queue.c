#include <stdlib.h>
#include <pthread.h>
#include "segel.h"

typedef struct node
{
    int data;
    struct node *next;
} node;

typedef struct queue
{
    int size;
    node *front, *back;
} queue;

struct queue *createQueue()
{
    struct queue *q = malloc(sizeof(struct queue));
    q->size = 0;
    q->back = q->front = NULL;
    return q;
}

void enqueue_noLock(struct queue *q, int data)
{
    node *n = (struct node *)malloc(sizeof(struct node));
    n->next = NULL;
    n->data = data;

    if (q->back != NULL)
    {
        q->back->next = n;
    }

    q->back = n;
    q->size += 1;
}

void enqueue(struct queue *q, int data, pthread_mutex_t *m, pthread_cond_t *c)
{
    pthread_mutex_lock(m);

    enqueue_noLock(q, data);

    pthread_cond_signal(c);

    pthread_mutex_unlock(m);
}

int dequeue_noLock(struct queue *q)
{
    node *head = q->front;
    q->front = q->front->next;
    q->size -= 1;

    int data = head->data;
    free(head);

    return data;
}

int dequeue(struct queue *q, pthread_mutex_t *m, pthread_cond_t *c)
{
    pthread_mutex_lock(m);
    int data = dequeue_noLock(q);
    pthread_mutex_unlock(m);
    return data;
}

int size(struct queue *q)
{
    return q->size;
}