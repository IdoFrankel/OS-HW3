#ifndef __QUEUE_H__

struct queue *createQueue(int maxSize);

void enqueue_noLock(struct queue *, int, unsigned long int);
int dequeue_noLock(struct queue *,unsigned long int* arrivel);
int dequeueByFd(struct queue *, int);

int dequeueByOrder(struct queue *, int);

int size(struct queue *);
int maxSize(struct queue *);

#endif
