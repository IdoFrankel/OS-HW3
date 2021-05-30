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
    printf("1\n");
    node *n = (struct node *)malloc(sizeof(struct node));
    n->next = NULL;
    n->data = data;

    printf("2\n");
    if (q->front == NULL)
    {
        q->front = n;
    }

    if (q->back != NULL)
    {
        q->back->next = n;
    }

    printf("3\n");
    q->back = n;
    q->size += 1;
    printf("4\n");
}

void enqueue(struct queue *q, int data, pthread_mutex_t *m, pthread_cond_t *c)
{
    printf("5\n");
    pthread_mutex_lock(m);

    printf("6\n");
    enqueue_noLock(q, data);

    printf("7\n");

    pthread_cond_signal(c);

    printf("8\n");

    pthread_mutex_unlock(m);
}

int dequeue_noLock(struct queue *q)
{
    printf("9\n");

    node *head = q->front;
    printf("9.1\n");

    q->front = q->front->next;

    printf("9.2\n");
    q->size -= 1;
    printf("10\n");

    int data = head->data;
    printf("10.5\n");

    free(head);

    printf("11\n");

    return data;
}

int dequeue(struct queue *q, pthread_mutex_t *m, pthread_cond_t *c)
{
    printf("12\n");
    pthread_mutex_lock(m);
    printf("13\n");

    while (q->size == 0)
    {
        printf("13.5\n");
        pthread_cond_wait(c, m);
    }

    int data = dequeue_noLock(q);
    printf("14\n");

    pthread_cond_signal(c);

    printf("15\n");

    pthread_mutex_unlock(m);

    printf("16\n");

    return data;
}

int size(struct queue *q)
{
    return q->size;
}