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
    printf("ttid = %d |\t queue.c 3\n", gettid());
    node *n = (struct node *)malloc(sizeof(struct node));
    n->next = NULL;
    n->prev = NULL;
    n->data = data;

    printf("ttid = %d |\t queue.c 4\n", gettid());
    if (q->front == NULL)
    {
        q->front = n;
    }

    if (q->back != NULL)
    {
        n->prev = q->back;
        q->back->next = n;
    }

    printf("ttid = %d |\t queue.c 5\n", gettid());
    q->back = n;
    q->size += 1;
    printf("ttid = %d |\t queue.c 6\n", gettid());
}

void enqueue(struct queue *q, int data, pthread_mutex_t *m, pthread_cond_t *c)
{
    printf("ttid = %d |\t queue.c 1\n", gettid());
    pthread_mutex_lock(m);

    printf("ttid = %d |\t queue.c 2\n", gettid());
    enqueue_noLock(q, data);

    // printf("ttid = %d |\t queue.c 7\n", gettid());

    pthread_cond_signal(c);

    // printf("ttid = %d |\t queue.c 8\n", gettid());

    pthread_mutex_unlock(m);
}

int dequeue_noLock(struct queue *q)
{
    printf("ttid = %d |\t queue.c 12\n", gettid());

    node *old_head = q->front;
    printf("ttid = %d |\t queue.c 12.1\n", gettid());

    q->front = q->front->next;
    if (q->front != NULL)
    {
        q->front->prev = NULL;
    }

    printf("ttid = %d |\t queue.c 12.2\n", gettid());
    q->size -= 1;
    printf("ttid = %d |\t queue.c 12.3\n", gettid());

    int data = old_head->data;
    printf("ttid = %d |\t queue.c 12.4\n", gettid());

    free(old_head);

    printf("ttid = %d |\t queue.c 12.5\n", gettid());

    return data;
}

int dequeue(struct queue *q, pthread_mutex_t *m, pthread_cond_t *c)
{
    printf("ttid = %d |\t queue.c 9\n", gettid());
    pthread_mutex_lock(m);
    printf("ttid = %d |\t queue.c 10\n", gettid());

    while (q->size == 0)
    {
        printf("ttid = %d |\t queue.c 11\n", gettid());
        pthread_cond_wait(c, m);
    }

    int data = dequeue_noLock(q);
    // printf("ttid = %d |\t queue.c 13\n", gettid());

    pthread_cond_signal(c);

    // printf("ttid = %d |\t queue.c 14\n", gettid());

    pthread_mutex_unlock(m);

    printf("ttid = %d |\t queue.c 16\n", gettid());

    return data;
}

int dequeueByFd(struct queue *q, pthread_mutex_t *m, pthread_cond_t *c, int fd)
{
    printf("ttid = %d |\t queue.c 17\n", gettid());
    pthread_mutex_lock(m);
    printf("ttid = %d |\t queue.c 18\n", gettid());

    struct node *n = q->front;
    while (n->next != NULL && n->data != fd)
    {
        n = n->next;
    }

    if (n->next == NULL && n->data != fd)
    {
        char content[MAXBUF];
        // *** BUG ***
        printf("BUGGGGG");
        sprintf(content, "%s %d wasnt inside the queue\n", content, fd);
        app_error(content);
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

        printf("signal running_insert_allow\n");
        q->size -= 1;

        pthread_cond_signal(c);
        int returnedFd = n->data;

        pthread_mutex_unlock(m);

        free(n);
        return returnedFd;
    }
}

// TODO shpould be locked?
// IF YES, THERE IS A PROBLEM, SINCE THERE ARE CALSL TO SIZE WITHIN LOCKS
// IN THAT CASE, RE LOCK WILL CAUSE ERROR, PROBLEM !!
int size(struct queue *q)
{
    int size = q->size;
    return size;
}