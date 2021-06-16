#ifndef __stats_H__
#include <sys/time.h>

struct req_stats* createReqstats();

struct thread_stats* createThreadstats(int id);

struct stats* createstats(int id);

/*unsigned long int getArrivelTime(struct stats* s);

void setArrivelTime(struct stats* s, unsigned long int arrivel);

unsigned long int getDispatchTime(struct stats* s);

void setDispatchTime(struct stats* s, unsigned long int dispatch);*/

struct timeval* getArrivelTime(struct stats* s);

void setArrivelTime(struct stats* s, struct timeval* arrivel);

struct timeval* getDispatchTime(struct stats* s);

void setDispatchTime(struct stats* s, struct timeval* dispatch);

int getCount(struct stats* s);

void incCount(struct stats* s);

int getStatic(struct stats* s);

void incStatic(struct stats* s);

int getDynamic(struct stats* s);

void incDynamic(struct stats* s);

int getId(struct stats* s);

#endif
