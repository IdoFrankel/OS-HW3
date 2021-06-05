#ifndef __QUEUE_H__

struct queue *createQueue(int maxSize);

void enqueue_noLock(struct queue *, int);
int dequeue_noLock(struct queue *);
int dequeueByFd(struct queue *, int);

int size(struct queue *);
int maxSize(struct queue *);

#endif
