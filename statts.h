#ifndef __STATTS_H__


struct req_statts* createReqStatts();

struct thread_statts* createThreadStatts(int id);

struct statts* createStatts(int id);

unsigned long getArrivelTime(struct statts* s);

void setArrivelTime(struct statts* s, unsigned long arrivel);

unsigned long getDispatchTime(struct statts* s);

void setDispatchTime(struct statts* s, unsigned long dispatch);

int getCount(struct statts* s);

void incCount(struct statts* s);

int getStatic(struct statts* s);

void incStatic(struct statts* s);

int getDynamic(struct statts* s);

void incDynamic(struct statts* s);

int getId(struct statts* s);

#endif
