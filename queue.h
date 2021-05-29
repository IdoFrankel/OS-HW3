#ifndef __QUEUE_H__

struct queue *createQueue();
void enqueue(struct queue *, int, pthread_mutex_t *, pthread_cond_t *);
void enqueue_noLock(struct queue *, int);

int dequeue(struct queue *, pthread_mutex_t *, pthread_cond_t *);
int dequeue_noLock(struct queue *);
int size(struct queue *);

#endif
