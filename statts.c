#include <stdlib.h>

typedef struct req_statts
{
   unsigned long arrivel_time;
   unsigned long dispatch_time;

} req_statts;

typedef struct thread_statts
{
   int thread_id;
   int thread_count;
   int thread_static;
   int thread_dynamic;

} thread_statts;

typedef struct statts
{
    struct req_statts* rs;
    struct thread_statts* ts;
} statts;

struct req_statts* createReqStatts(){
    struct req_statts *rs = malloc(sizeof(*rs));
    rs->arrivel_time = -1;
    rs->dispatch_time = -1;
    return rs;
}

struct thread_statts* createThreadStatts(int id){
    struct thread_statts *ts = malloc(sizeof(*ts));
    ts->thread_id = id;
    ts->thread_count = 0;
    ts->thread_static = 0;
    ts->thread_dynamic = 0;
    return ts;
}

struct statts* createStatts(int id){
    struct statts *s = malloc(sizeof(*s));
    s->rs = createReqStatts();
    s->ts = createThreadStatts(id);
    return s;
}

unsigned long getArrivelTime(struct statts* s){
    return s->rs->arrivel_time;
}

void setArrivelTime(struct statts* s, unsigned long arrivel){
    s->rs->arrivel_time = arrivel;
}

unsigned long getDispatchTime(struct statts* s){
    return s->rs->dispatch_time;
}

void setDispatchTime(struct statts* s, unsigned long dispatch){
    s->rs->dispatch_time = dispatch;
}
int getCount(struct statts* s){
    return s->ts->thread_count;
}

void incCount(struct statts* s){
    s->ts->thread_count ++;
}

int getStatic(struct statts* s){
    return s->ts->thread_static;
}

void incStatic(struct statts* s){
    s->ts->thread_static ++;
}

int getDynamic(struct statts* s){
    return s->ts->thread_dynamic;
}

void incDynamic(struct statts* s){
    s->ts->thread_dynamic ++;
}

int getId(struct statts* s){
    return s->ts->thread_id;
}