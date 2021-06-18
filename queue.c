#include "segel.h"

typedef struct node
{
    int data;
    //unsigned long int request_arrivel;
    struct timeval *request_arrivel;
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

void enqueue_noLock(struct queue *q, int data, struct timeval *req_arrivel)
{
    node *n = (struct node *)malloc(sizeof(struct node));
    n->next = NULL;
    n->prev = NULL;
    n->data = data;
    n->request_arrivel = req_arrivel;

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

int dequeue_noLock(struct queue *q, struct timeval *arrivel)
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

    if (arrivel != NULL)
    {
        arrivel->tv_sec = old_head->request_arrivel->tv_sec;
        arrivel->tv_usec = old_head->request_arrivel->tv_usec;
    }

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

int dequeueByOrder(struct queue *q, int index)
{
    struct node *pos = q->front;
    int i = 1;
    while (i != index)
    {
        i++;
        pos = pos->next;
    }
    if (pos == q->front)
    {
        q->front = pos->next;
        (pos->next)->prev = NULL;
    }
    if (pos == q->back)
    {
        q->back = pos->prev;
        pos->prev->next = NULL;
    }

    if (pos->next != NULL)
    {
        (pos->next)->prev = pos->prev;
    }
    if (pos->prev != NULL)
    {
        (pos->prev)->next = pos->next;
    }

    int result = pos->data;
    pos->next = NULL;
    pos->prev = NULL;
    free(pos);
    q->size--;
    return result;
}
