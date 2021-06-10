#include <stdlib.h>

typedef struct req_stats
{
   unsigned long int arrivel_time;
   unsigned long int dispatch_time;

} req_stats;

typedef struct thread_stats
{
   int thread_id;
   int thread_count;
   int thread_static;
   int thread_dynamic;

} thread_stats;

typedef struct stats
{
    struct req_stats* rs;
    struct thread_stats* ts;
} stats;

struct req_stats* createReqstats(){
    struct req_stats *rs = malloc(sizeof(*rs));
    rs->arrivel_time = -1;
    rs->dispatch_time = -1;
    return rs;
}

struct thread_stats* createThreadstats(int id){
    struct thread_stats *ts = malloc(sizeof(*ts));
    ts->thread_id = id;
    ts->thread_count = 0;
    ts->thread_static = 0;
    ts->thread_dynamic = 0;
    return ts;
}

struct stats* createstats(int id){
    struct stats *s = malloc(sizeof(*s));
    s->rs = createReqstats();
    s->ts = createThreadstats(id);
    return s;
}

unsigned long int getArrivelTime(struct stats* s){
    return s->rs->arrivel_time;
}

void setArrivelTime(struct stats* s, unsigned long int arrivel){
    s->rs->arrivel_time = arrivel;
}

unsigned long int getDispatchTime(struct stats* s){
    return s->rs->dispatch_time;
}

void setDispatchTime(struct stats* s, unsigned long int dispatch){
    s->rs->dispatch_time = dispatch;
}
int getCount(struct stats* s){
    return s->ts->thread_count;
}

void incCount(struct stats* s){
    s->ts->thread_count ++;
}

int getStatic(struct stats* s){
    return s->ts->thread_static;
}

void incStatic(struct stats* s){
    s->ts->thread_static ++;
}

int getDynamic(struct stats* s){
    return s->ts->thread_dynamic;
}

void incDynamic(struct stats* s){
    s->ts->thread_dynamic ++;
}

int getId(struct stats* s){
    return s->ts->thread_id;
}