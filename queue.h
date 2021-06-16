#ifndef __QUEUE_H__
#include <sys/time.h>

struct queue *createQueue(int maxSize);

/*void enqueue_noLock(struct queue *, int, unsigned long int);
int dequeue_noLock(struct queue *,unsigned long int* arrivel);*/

void enqueue_noLock(struct queue *, int, struct timeval* arrivel);
int dequeue_noLock(struct queue *,struct timeval* arrivel);

int dequeueByFd(struct queue *, int);

int dequeueByOrder(struct queue *, int);

int size(struct queue *);
int maxSize(struct queue *);

#endif
